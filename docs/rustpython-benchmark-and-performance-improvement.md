# Benchmark and Performance Improvement of RustPython on WasmEdge

## What is RustPython and how it runs in WasmEdge?

[RustPython](https://github.com/RustPython/RustPython) is a Python runtime written in Rust. Since it is written in Rust, we can compile RustPython code easily into WebAssembly (wasm file) with cargo command.
```
cargo build --target wasm32-wasi --no-default-features --features freeze-stdlib,stdlib --release
```

A compiled RustPython wasm file can be downloaded in [rustpython.wasm](https://github.com/wenym1/WasmEdge/blob/master/rust-python-benchmark/rustpython.wasm).

WasmEdge is a WebAssembly runtime, and therefore the wasm file of RustPython can be run on WasmEdge with simply
```
wasmedge rustpython.wasm
```

## Building WasmEdge

We bench WasmEdge on MacOS with M1 chip. We can build WasmEdge following the instructions in [Build from source on MacOS](https://wasmedge.org/book/en/extend/build_on_mac.html). Since I am using MacOS 12.2.1, I met some problems related to MacOS SDK. The problem is described in [#1485](https://github.com/WasmEdge/WasmEdge/issues/1485). The solution will be adding a new option `-DCMAKE_OSX_SYSROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX11.3.sdk/` in `cmake` in enfore using an older version MacOS SDK.

## Benchmark

We need a benchmark to test the performance of running RustPython on WasmEdge. The benchmark code is purely Python code, and to run the benchmark, we can simply flush the benchmark Python code to `stdin` when we run command `wasmedge rustpython.wasm`. The benchmark code is compute-intensive Python code, which repeat some arithmetic operations such as integer and float add, multiplication, divide, and bitwise operations. The benchmark code can be found in [arithmatic-bench.py](https://github.com/wenym1/WasmEdge/blob/master/rust-python-benchmark/bench-src/arithmetic_bench.py)

## Profiling

### Flamegraph

WasmEdge has two runtime mode. The first one is interpreter mode, which loads the wasm file during runtime and interpret and execute each wasm instruction one by one. In our test, we benchmark RustPython in interpreter mode.

We profile the WasmEdge with `dtrace`. We can run the following command to start the benchmark and dump the runtime stacks sample to `wasmedge.rust-python.stacks` file.
```
cat rust-python-benchmark/arithmetic_bench.py | sudo dtrace -x 'ustackframes=100' -n 'profile-997 /pid == $target/ { @[ustack(100)] = count(); }' -o wasmedge.rust-python.stacks -c './build/tools/wasmedge/wasmedge rustpython.wasm'
```

The dummped runtime stacks sample can be converted into flamegraph with [FlameGraph](https://github.com/brendangregg/FlameGraph). On MacOS, we can use the following command
```
FlameGraph/stackcollapse.pl wasmedge.rust-python.stacks | FlameGraph/flamegraph.pl > wasmedge.rust-python.svg
```

A flamegraph generated during benchmark can be found in ![wasmedge.rust-python.svg](https://github.com/wenym1/WasmEdge/blob/master/rust-python-benchmark/result/wasmedge.rust-python.svg).

### Operation count

Since we are running benchmark in interpreter mode, we can easily count the number of each WebAssembly op during benchmark. We added a hashmap in WasmEdge runtime, recompile the code and run the benchmark code to collect the count of each op during benchmark. The result can be found in [op_code_count.txt](https://github.com/wenym1/WasmEdge/blob/master/rust-python-benchmark/result/op_code_count.txt)

## Evaluation

The benchmark takes about 119 seconds to finish on my Macbook. First we will take a look at the operation count. Operations with top count are listed below
|Op code | Op name | count |
|----|-----|---|
|2 |block| 3794881436|
|20| local.get | 3629560472 |
|41|i32.const |2068731072|
|6a|i32.add | 1241638174|
|22|local.tee | 951083474|
|d | br_if| 795455879|
|28 | i32.load| 686440934|
| b | end | 647265668 |
| 21 | local.set| 508795038|
|36 |i32.store| 453224938|
|37|i64.store| 233740492|
|71 |i32.and| 228313220|
|29 |i64.load| 212579460|
|42 | i64.const| 202053757 |
|45 | i32.eqz| 193674856|

We may notice that, except for flow control related operations, such as `block`, `end` and `br_if`, most of the top operations are memory and arithmatic operations. So the bottleneck of the system may be related to these operations, and we may improve the performance if we optimize these operations.

Next we can lookup at the flamegraph. From the flamegraph we can see that, most time is spent with running the `WasmEdge::Executor::Executor::execute` method. This is as expected because we are running in interpreter mode, and the WebAssembly interpretation and execution are working in a while loop in the method. We can also notice that, the method `runStoreOp` and `runLoadOp` take non-negligible amount of time. Previously we have noticed that memory operations are one of the top operations in our benchmark, and therefore, our observation is reasonable. And we may be able to optimize the `runStoreOp` and `runLoadOp` to optimize the benchmark performance.

## Code Optimization

### Code analysis and initial optimization

First we shall look at the code of the two methods. We will take `runStoreOp` as an example.

The code of `runStoreOp` is as followed
```
template <typename T>
TypeN<T> Executor::runStoreOp(Runtime::StackManager &StackMgr,
                              Runtime::Instance::MemoryInstance &MemInst,
                              const AST::Instruction &Instr,
                              const uint32_t BitWidth) {
  // Pop the value t.const c from the Stack
  T C = StackMgr.pop().get<T>();

  // Calculate EA = i + offset
  uint32_t I = StackMgr.pop().get<uint32_t>();
  if (I > std::numeric_limits<uint32_t>::max() - Instr.getMemoryOffset()) {
    spdlog::error(ErrCode::MemoryOutOfBounds);
    spdlog::error(ErrInfo::InfoBoundary(
        I + static_cast<uint64_t>(Instr.getMemoryOffset()), BitWidth / 8,
        MemInst.getBoundIdx()));
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::MemoryOutOfBounds);
  }
  uint32_t EA = I + Instr.getMemoryOffset();

  // Store value to bytes.
  if (auto Res = MemInst.storeValue(C, EA, BitWidth / 8); !Res) {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(Res);
  }
  return {};
}
```

We can see that it does two works: boundary check and memory storing. Since we didn't get any error in benchmark, we may assume that while execution we pass all boundary checks. Boundary checks are simply integer comparison and should not take too much time. Then we should focus on `MemInst.storeValue`. The code of `MemInst.storeValue` is as followed.
```
template <typename T>
  typename std::enable_if_t<IsWasmNativeNumV<T>, Expect<void>>
  storeValue(const T &Value, uint32_t Offset, uint32_t Length) noexcept {
    // Check the data boundary.
    if (unlikely(Length > sizeof(T))) {
      spdlog::error(ErrCode::MemoryOutOfBounds);
      spdlog::error(
          ErrInfo::InfoBoundary(Offset, Length, Offset + sizeof(T) - 1));
      return Unexpect(ErrCode::MemoryOutOfBounds);
    }
    // Check the memory boundary.
    if (unlikely(!checkAccessBound(Offset, Length))) {
      spdlog::error(ErrCode::MemoryOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, getBoundIdx()));
      return Unexpect(ErrCode::MemoryOutOfBounds);
    }
    // Copy the stored data to the value.
    if (likely(Length > 0)) {
      std::memcpy(&DataPtr[Offset], &Value, Length);
    }
    return {};
  }
```
The method does length validity check, boundary check and then does `memcpy`. Length validity check and boundary check are both integer comparison and should not take too much time. Therefore, the main bottleneck might be `memcpy`.

We can check the [source code of memcpy](https://code.woboq.org/gcc/libgcc/memcpy.c.html)
```
void *
memcpy (void *dest, const void *src, size_t len)
{
  char *d = dest;
  const char *s = src;
  while (len--)
    *d++ = *s++;
  return dest;
}
```
Its implementation is simply reading and writing byte one by one, which is not efficient, since modern CPU instruction supports loading and storing memory word by word.

Besides, we check all usage of `runStoreOp` and noticed that all caller of `runStoreOp` can know the `BitWidth` parameter at compile time. `BitWidth / 8` is the `Length` parameter of `storeValue`. We also checked all usage of `storeValue` and again noticed that all caller can know its `Length `parameter at compile time. Therefore, we can move the `Length` parameter from a method parameter to a template argument so that the compiler can have more optimization statically at compile time. Besides, since the `Length` parameter can be known at compile time, the length validity check can also be performed at compile time with `static_assert`, and we save an integer comparison at runtime and avoid a potential branch prediction miss. We may also want to leverage the ability of modern CPU to load the store memory word by word. We notice that the `Length` in `storeValue` are mostly the power of 2, such as 1, 2, 4, 8, 16 and etc. Therefore, we may develop a more effective `memcpy` by loading and storing more than one bytes at a time. A more effective `memcpy` can be like
```
template <uint32_t Length> void effective_memcpy(void *dest, const void *src) {
  switch (Length) {
  case 1:
    *reinterpret_cast<volatile uint8_t *>(dest) =
        *reinterpret_cast<const volatile uint8_t *>(src);
    break;
  case 2:
    *reinterpret_cast<volatile uint16_t *>(dest) =
        *reinterpret_cast<const volatile uint16_t *>(src);
    break;
  case 4:
    *reinterpret_cast<volatile uint32_t *>(dest) =
        *reinterpret_cast<const volatile uint32_t *>(src);
    break;
  case 8:
    *reinterpret_cast<volatile uint64_t *>(dest) =
        *reinterpret_cast<const volatile uint64_t *>(src);
    break;
  case 16:
    *reinterpret_cast<volatile uint128_t *>(dest) =
        *reinterpret_cast<const volatile uint128_t *>(src);
    break;
  default:
    std::memcpy(dest, src, Length);
    break;
  }
}
```
We did the same change to `loadValue` as well. By moving `Length` to template argument and replace `memcpy` with `effective_memcpy`, the benchmark runtime reduces from 119 seconds to 111 seconds, which is exciting!

### Submiting PR

We submitted a PR to request merging our optimized code to master branch. The PR was [#1507](https://github.com/WasmEdge/WasmEdge/pull/1507). The CI failed when we use GCC on Ubuntu in release mode. As suggested by the code reviewer, the `reinterpret_cast` used in `effective_memcpy` might not be aligned, and may break the alignment assumption when the compiler is optimizing the code. Therefore I added an alignment check and for the unaligned memory load and store, we still use `memcpy`.

The modified `effective_memcpy` is as followed.
```
template <uint32_t Length> void effective_memcpy(void *dest, const void *src) {
  if constexpr (Length == 0) {
    return;
  }
  // If Length is not the power of 2, use memcpy
  if constexpr (((Length) & (Length - 1)) != 0) {
    std::memcpy(dest, src, Length);
    return;
  }
  // Check alignment of dest and src to Length. If any of dest and src is not
  // aligned, use memcpy
  const uint32_t alignment_mask = Length - 1;
  if ((reinterpret_cast<uintptr_t>(dest) & alignment_mask) ||
      (reinterpret_cast<uintptr_t>(src) & alignment_mask)) {
    std::memcpy(dest, src, Length);
    return;
  }
  switch (Length) {
  case 1:
    *reinterpret_cast<volatile uint8_t *>(dest) =
        *reinterpret_cast<const volatile uint8_t *>(src);
    break;
  case 2:
    *reinterpret_cast<volatile uint16_t *>(dest) =
        *reinterpret_cast<const volatile uint16_t *>(src);
    break;
  case 4:
    *reinterpret_cast<volatile uint32_t *>(dest) =
        *reinterpret_cast<const volatile uint32_t *>(src);
    break;
  case 8:
    *reinterpret_cast<volatile uint64_t *>(dest) =
        *reinterpret_cast<const volatile uint64_t *>(src);
    break;
  case 16:
    *reinterpret_cast<volatile uint128_t *>(dest) =
        *reinterpret_cast<const volatile uint128_t *>(src);
    break;
  default:
    std::memcpy(dest, src, Length);
    break;
  }
}
```

With the alignment check, the CI is passed, while the performance is not impacted.

### The magic of memcpy optimization

After we added alignment check and pass the CI, the code reviewer raise a question: is our `effective_memcpy` really helpful? 

The PR does two things: move `Length` as a template argument and introduce `effective_memcpy`. We may do some experiments to justify whether the `effective_memcpy` is the reason of improvement.

4 set of experiments are conducted and the result is as followed.
| time | run1 | run2 | run3 | avg |
| --- | --- | --- | --- |  --- |
|Current | 118.08s | 117.88s | 118.87s | 118.28s |
|Current with effective_memcpy| 116.11s | 115.41s | 115.69s | 115.74s|
| const template with std::memcpy | 111.03s | 111.15s | 111.38s | 111.19s |
| const template with effective_memcpy | 110.87s | 111.10s | 112.04s | 111.34s| 

From the experiment result, we can notice that as long as we move `Length` as the template argument, we can reduce the benchmark time to 111 seconds regardless whether we use `effective_memcpy` or not, and even the performance of `effective_memcpy` is a little worse than `memcpy`. The only possible explanation is that the compiler can do some optimization on `memcpy` when it knows the length at compile time. 

We take a try on [Compiler explorer](godbolt.org), which compile the C++ code online and shows the assembly instructions.

We enter the following code to test whether the compiler can do optimization.
```
#include <string.h>

void test_const_length(void* dest, void* src) {
    memcpy(dest, src, 8);
}

void test_dynamic_length(void* dest, void* src, int length) {
    memcpy(dest, src, length);
}
```

The compiled assembly code is as followed.
```
test_const_length(void*, void*):              # @test_const_length(void*, void*)
        push    rbp
        mov     rbp, rsp
        mov     qword ptr [rbp - 8], rdi
        mov     qword ptr [rbp - 16], rsi
        mov     rax, qword ptr [rbp - 8]
        mov     rcx, qword ptr [rbp - 16]
        mov     rcx, qword ptr [rcx]
        mov     qword ptr [rax], rcx
        pop     rbp
        ret
test_dynamic_length(void*, void*, int):           # @test_dynamic_length(void*, void*, int)
        push    rbp
        mov     rbp, rsp
        sub     rsp, 32
        mov     qword ptr [rbp - 8], rdi
        mov     qword ptr [rbp - 16], rsi
        mov     dword ptr [rbp - 20], edx
        mov     rdi, qword ptr [rbp - 8]
        mov     rsi, qword ptr [rbp - 16]
        movsxd  rdx, dword ptr [rbp - 20]
        call    memcpy@PLT
        add     rsp, 32
        pop     rbp
        ret
```

From the assembly code, we can see that for `test_dynamic_length` whose length cannot be known at compile time, it calls the real `memcpy`. However, for `test_const_length` whose length can be known to be 8 at compile time, instead of calling `memcpy`, the code will load the 8-byte word to register and store the 8 bytes to the target memory address all at once, which is exactly what `effective_memcpy` does, and the compiler optimization can achieve it more elegantly. So our guess is correct, the compiler did optimize `memcpy` when it knows the length at compile time!

So in conclusion, we don't need `effective_memcpy`. We only need to let the compile know the length at compile time by moving the `Length` as a template argument. After I made the final change, the PR is merged!
