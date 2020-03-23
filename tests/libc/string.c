#include <libc/stdio.h>
#include <libc/string.h>

int test_strlen() {
    return strlen("") != 0 &&
        strlen("a") == 1 &&
        strlen("aaaaaaaaa") == 9;
}

#define NTEST 1
typedef int (*test_func)();
static test_func test_funcs[NTEST] = {
    test_strlen
};

int main(void) {
    for (size_t i = 0; i < NTEST; i++) {
        if (!test_funcs[i]()) return 1;
    }

    return 0;
}
