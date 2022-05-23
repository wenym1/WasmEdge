# Integer
## Add

def int_for_loop_sum():
    sum = 0
    for i in range(100000):
        sum += i
    return sum

def big_decimal_add():
    sum = 0
    first_base = 1003423219473912740938
    second_base = 31439740931748932714789571894
    for _ in range(100000):
        first_base += 1
        second_base += 1
        sum += first_base + second_base
    return sum

## Multiply
def factorial():
    mul = 1
    for i in range(1, 1000):
        mul *= i
    return mul

## Divide and mod
def __gcd_inner(first, second):
    mod = first % second
    if mod == 0:
        return second
    else:
        return __gcd_inner(second, mod)

def gcd():
    return __gcd_inner(1003423219473912740938, 31439740931748932714789571893)

## bit operation
def bit_operation():
    first = 0x1234abcd
    second = 0xabcd1234
    for _ in range(100000):
        first & second
        first | second
        first ^ second
        ~first
        ~second


# Floating point
## Add
def float_for_loop_sum():
    sum = 0.0
    base_num = 1.0
    for _ in range(100000):
        sum += base_num
        base_num += 1.0
    return sum

## Multiply
def float_multiply():
    sum = 0.0
    first = 1.12345678
    second = 3.1415926
    for _ in range(100000):
        sum += first * second
    return sum

## Divide
def float_divide():
    sum = 0.0
    first = 1.12345678
    second = 3.1415926
    for _ in range(100000):
        sum += first / second
    return sum

tests = [(name, func) for (name, func) in globals().items() if not name.startswith('__')]

def run_tests(tests, iter=5):
    import time
    start_time = time.time()
    for i in range(iter):
        print("Start running iter", i + 1)
        for (name, func) in tests:
            print("Running", name)
            func()
            print("Finish running", name)
        print("Finish running iter", i + 1)
    print("Finish running all. Time: ", time.time() - start_time)

run_tests(tests)
