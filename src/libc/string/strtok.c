#include <libc/string.h>

static char *str = 0;

char *strtok(char *src, const char *delim) {
    if (src) str = src;
    if (!*str) return str;

    char *ptr = str, *c;

    while (*delim) {
        if (!*(c = index(str, *delim))) {
            *c = 0;
            str = c+1;
        }

        delim++;
    }

    return ptr;
}
