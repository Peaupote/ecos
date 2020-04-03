#include <libc/string.h>

int strncmp(const char *lhs, const char *rhs, size_t len) {
	if (!len) return 0;
	if (*lhs < *rhs) return -1;
	if (*lhs > *rhs) return  1;
	if (!*lhs) return 0;
    while(len-- && *lhs) {
        ++lhs;
        ++rhs;
		if (*lhs < *rhs) return -1;
		if (*lhs > *rhs) return  1;
    }
	return 0;
}
