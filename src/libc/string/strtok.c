#include <libc/string.h>

static char *str = 0;

char *strtok(char *src, const char *delim) {
    if (src) str = src;
    if (!str || !*str) return 0;

    char *ptr = str, *c;

    while (*delim) {
        c = index(str, *delim);
        while(c[1] == *delim) c++;

        if (c) {
            *c = 0;
            str = c+1;
            return ptr;
        }

        delim++;
    }

    str = 0;
    return ptr;
}
