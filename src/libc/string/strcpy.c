#include <libc/string.h>

char *strcpy(char *dst_bg, const char *src) {
    for (char *dst = dst_bg; (*dst++ = *src); ++src);
    return dst_bg;
}
