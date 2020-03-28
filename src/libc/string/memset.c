#include <libc/string.h>

void *memset(void *pdst, int c, size_t len) {
    char *dst = (char*)pdst;
    while (len--) *dst++ = c;
    return pdst;
}
