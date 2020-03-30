#include <libc/string.h>
#include <libc/stdio.h>

size_t TEST_D(strlen)(const char *str) {
    char *s = (char*)str;
    size_t l = 0;
    while (*s++) l++;
    return l;
}
