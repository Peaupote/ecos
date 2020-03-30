#include <libc/string.h>

int TEST_D(strncmp)(const char *lhs, const char *rhs, size_t len) {
    while(len-- && *lhs && *rhs){
        if ((unsigned char)*lhs < (unsigned char)*rhs)
            return -1;
        if ((unsigned char)*lhs > (unsigned char)*rhs)
            return 1;
        ++lhs;
        ++rhs;
    }
    if(*rhs) return -1;
    if(*lhs) return  1;
    return 0;
}
