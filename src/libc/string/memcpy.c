#include <libc/string.h>

void *memcpy(void *dst_bg, const void *src_bg, size_t len) {
    for(char *src = (char*)src_bg, *dst = (char*)dst_bg; len; --len)
		*dst++ = *src++;
    return dst_bg;
}
