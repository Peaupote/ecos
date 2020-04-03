#include <libc/string.h>

char *strchrnul(const char *s, int c) {
    if (*s == c) return (char*)s;
    while (*s) {
        s++;
        if (*s == c) return (char*)s;
    }
    return (char*)s;
}
