#include <string.h>

void *memcpy(void *pdst, const void *psrc, size_t len) {
    char *src = (char*)psrc, *dst = (char*)pdst;
    for (;len > 0; len--) *dst++ = *src++;
    return pdst;
}
