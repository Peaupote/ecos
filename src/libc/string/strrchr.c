#include <libc/string.h>

char *strrchr(const char *s, int c) {
    char *ret = 0;
    for (; *s; ++s) {
        if (*s == c) ret = (char*)s;
    }

    return ret;
}
