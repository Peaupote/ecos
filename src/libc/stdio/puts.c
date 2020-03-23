#include <libc/stdio.h>

int puts(const char *s) {
    return printf("%s", s);
}
