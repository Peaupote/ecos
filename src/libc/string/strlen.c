#include <libc/string.h>
#include <libc/stdio.h>

size_t strlen(const char *s_bg) {
    char *s = (char*)s_bg;
    while (*s) s++;
    return s - s_bg;
}
