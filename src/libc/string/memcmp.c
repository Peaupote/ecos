#include <libc/string.h>

int memcmp(const void *l, const void *r, size_t len) {
	const unsigned char *lc = (const unsigned char*) l,
		  *rc = (const unsigned char*) r;
    while(len--){
        if (*lc < *rc)
            return -1;
        if (*lc > *rc)
            return 1;
        ++lc;
        ++rc;
    }
    return 0;
}
