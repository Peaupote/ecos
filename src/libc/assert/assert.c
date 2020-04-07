#include <libc/assert.h>
#include <libc/stdio.h>
#include <libc/stdlib.h>

void print_assert(int p, const char *file, const char *func, int line) {
    if (!p) {
        printf("%s: %s:%d: Assertion failed", file, func, line);
        exit(1);
    }
}
