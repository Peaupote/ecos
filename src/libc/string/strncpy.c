#include <libc/string.h>

char *strncpy(char *dst, const char *src, size_t len) {
    char *d = dst;
    while (len-- && (*dst++ = *src++));
    return d;
}
