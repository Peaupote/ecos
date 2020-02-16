#include <string.h>

void *memset(void *pdst, int c, size_t len) {
    char *dst = (char*)pdst;
    for (; len > 0; len--) *dst++ = c;
    return pdst;
}
