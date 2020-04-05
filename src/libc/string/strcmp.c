#include <libc/string.h>

int strcmp(const char *lhs, const char *rhs){
    const unsigned char *l = (const unsigned char*)lhs;
    const unsigned char *r = (const unsigned char*)rhs;

    while (*l && (*l == *r)) ++l, ++r;

    return *l - *r;
}
