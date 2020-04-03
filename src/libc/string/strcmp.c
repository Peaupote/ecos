#include <libc/string.h>

int strcmp(const char *lhs, const char *rhs){
	if (*lhs < *rhs) return -1;
	if (*lhs > *rhs) return  1;
    while(*lhs) {
        ++lhs;
        ++rhs;
        if (*lhs < *rhs) return -1;
        if (*lhs > *rhs) return  1;
    }
    return 0;
}
