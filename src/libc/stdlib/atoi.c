#include <libc/stdlib.h>

int atoi(const char *s) {
    int n;
    for(n = 0; *s; s++)
        n = n * 10 + (*s - '0');

    return n;
}
