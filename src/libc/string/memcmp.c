#include <libc/string.h>

int memcmp(const void *l, const void *r, size_t len) {
    return strncmp((char*)l, (char*)r, len);
}
