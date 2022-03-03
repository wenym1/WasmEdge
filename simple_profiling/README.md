# Simple Profiling on Matrix Multiplication

This module is for the challenge task described in [Mentorship Challenge - Profiling #1180](https://github.com/WasmEdge/WasmEdge/discussions/1180). 

## Matrix Multiplication Test Program

We implemented simple matrix multiplication in a rust project named `matrix-mul`. To build and run a binary, run
```
cargo run ${row_size} ${internal_dim_size} ${col_size}
```
, where `${row_size}` is the number of row in the first matrix, `${internal_dim_size}` is the number of column in the first matrix and the number of row in the second matrix, and `${col_size}` is the number of columns in the second matrix.

We can generate the wasm file by running the following command under `matrix-mul`.
```
cargo build --target wasm32-wasi
```
The output wasm file is `target/wasm32-wasi/debug/matrix-mul.wasm`. We will run the following command for profiling, which multiplies two matrices with size 100x100.
```
wasmedge target/wasm32-wasi/debug/matrix-mul.wasm 100 100 100
```

## Build WasmEdge with `gprof` Profiling

We will use `gprof` to profile the program.

The WasmEdge program can be built following [build](https://wasmedge.org/book/en/extend/build.html).

Since we want to use `gprof`, when we run `cmake` to initialize the build directory, we should add `-DCMAKE_CXX_FLAGS=-pg` option so that we can have the `-pg` option when we build the app. The `cmake` command to initialize the build directory is like
```
cmake -DCMAKE_CXX_FLAGS=-pg -DWASMEDGE_BUILD_TESTS=ON -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON -DWASMEDGE_BUILD_AOT_RUNTIME=OFF ..
```

## Run and Profiling with `gprof`
After building WasmEdge, we can run the matrix multiplication program WasmEdge VM and do profiling.

Under the build directory of WasmEdge, we can run
```
./tools/wasmedge/wasmedge ../simple_profiling/matrix-mul/target/wasm32-wasi/debug/matrix-mul.wasm  100 100 100
gprof ./tools/wasmedge/wasmedge > gprof-run.txt
```
The profiling result is in `gprof-run.txt`. We can create the callgraph dot file with [gprof2dot](https://github.com/jrfonseca/gprof2dot) tools and further generate a `svg` format callgraph.

A group of sample output file is as followed.
 - [wasm file for matrix multiplication](./matrix-mul.wasm)
 - [gprof output file](./matrix-mul-100-100-100.txt)
 - [callgraph dot file](./matrix-mul-100-100-100.dot)
 - [callgraph.svg](./matrix-mul-100-100-100.svg)
