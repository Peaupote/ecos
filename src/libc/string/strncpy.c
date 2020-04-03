#include <libc/string.h>

char *strncpy(char *dst, const char *src, size_t len) {
    for(char* d = dst; len-- && (*d++ = *src++); );
    return dst;
}
