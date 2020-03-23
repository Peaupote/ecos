#include <libc/string.h>

void *memmove(void *pdst, const void *psrc, size_t len) {
    char *dst = (char*)pdst, *src = (char*)psrc;
    if (psrc > pdst) return memcpy(pdst, psrc, len);

    // copy from end to beginning
    dst += len;
    src += len;
    for (;len > 0; len--) *--dst = *--src;
    return dst;
}
