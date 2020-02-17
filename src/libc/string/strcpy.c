#include <string.h>

char *strcpy(char *pdst, const char *psrc) {
    for (char *src = (char*)psrc, *dst = pdst; *src; *dst++ = *src++);
    return pdst;
}
