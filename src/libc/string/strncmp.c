#include <libc/string.h>

int strncmp(const char *lhs, const char *rhs, size_t len) {
    const unsigned char *l = (const unsigned char*)lhs;
    const unsigned char *r = (const unsigned char*)rhs;

    while (len && *l && *l == *r) --len, ++l, ++r;

    if (len > 0) return *l - *r;
    return 0;
}
