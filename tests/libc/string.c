#include <libc/stdio.h>
#include <libc/string.h>

int test_strlen() {
    return
        strlen("") == 0 &&
        strlen("a") == 1 &&
        strlen("aaaaaaaaa") == 9;
}

int test_memcmp() {
    char *t1 = "abc",
        *t2 = "aaa";

    return
        !memcmp(t1, t2, 0) &&
        !memcmp(t1, t2, 1) &&
        memcmp(t1, t2, 3) > 0 &&
        memcmp(t2, t1, 3) < 0;
}

#define NTEST 2
static char *test_name[NTEST] = {
    "memcmp",
    "strlen"
};

typedef int (*test_func)();
static test_func test_funcs[NTEST] = {
    test_memcmp,
    test_strlen
};

typedef int (*writer)(const char *fmt, ...);
#if defined(__is_kernel)
#include <kernel/kutil.h>
static writer pp = kprintf;
#else
static writer pp = printf;
#endif

#if defined(__is_kernel)
int test_string(void) {
#else
int main(void) {
#endif

    pp("Test libc string.h\n");

    int rc = 0;
    for (size_t i = 0; i < NTEST; i++) {
        pp("test %s... ", test_name[i]);
        if (!test_funcs[i]()) {
            rc = 1;
            pp("FAILED\n");
        } else pp("OK\n");
    }

    return rc;
}
