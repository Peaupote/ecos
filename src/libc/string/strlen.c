#include <libc/string.h>
#include <libc/stdio.h>

size_t strlen(const char *str) {
    printf("coucou\n");
    char *s = (char*)str;
    size_t l = 0;
    while (*s++) l++;
    return l;
}
