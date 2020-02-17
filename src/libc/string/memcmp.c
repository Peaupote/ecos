#include <string.h>

int memcmp(const void *srcp, const void *dstp, size_t len) {
    char *src = (char*)srcp, *dst = (char*)dstp;
    int r = 0;
    while (len > 0 && (r = (*src++ == *dst++))) len--;
    return r;
}
