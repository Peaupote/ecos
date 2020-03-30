#include <libc/string.h>
#include <libc/stdio.h>

size_t strlen(const char *str) {
    char *s = (char*)str;
    size_t l = 0;
    while (*s++) l++;
    return l;
}
