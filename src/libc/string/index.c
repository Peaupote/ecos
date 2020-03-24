#include <libc/string.h>

char *index(const char *src, int c) {
    while (*src && *src != c) src++;
    return (char*)src;
}
