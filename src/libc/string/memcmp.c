#include <libc/string.h>

int TEST_D(memcmp)(const void *l, const void *r, size_t len) {
    return TEST_D(strncmp)((char*)l, (char*)r, len);
}
