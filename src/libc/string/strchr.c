#include <libc/string.h>

char *strchr(const char *s, int c) {
    char *p = 0;
    while (*s) {
        if (*s == c) p = (char*)s;
        s++;
    }

    return p;
}
