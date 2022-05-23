LOOP_COUNT = 100000

FIRST = "first"
SECOND = "second"
FIRST_SECOND = "first_second"


def run_multi_time(func):
    def wrapper():
        for _ in range(LOOP_COUNT):
            func()
    return wrapper


@run_multi_time
def concat_test():
    FIRST + SECOND


@run_multi_time
def contains_test():
    SECOND in FIRST
    FIRST in FIRST_SECOND


@run_multi_time
def eq_test():
    FIRST_SECOND == FIRST + "_" + SECOND


@run_multi_time
def compare_test():
    FIRST > SECOND
    FIRST_SECOND < SECOND


@run_multi_time
def take_char_test():
    FIRST[3]
    SECOND[2]


@run_multi_time
def take_slice_test():
    FIRST_SECOND[3:8]


@run_multi_time
def repeat_test():
    FIRST_SECOND * 3


@run_multi_time
def capitalize_test():
    FIRST_SECOND.capitalize()


@run_multi_time
def count_test():
    (FIRST_SECOND * 3).count("i")


if __name__ == "__main__":
    global_var = {}
    global_var.update(globals())
    for name in global_var:
        if name.endswith("_test"):
            global_var[name]()
