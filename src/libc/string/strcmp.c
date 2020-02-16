#include <string.h>

int strcmp(const char *lhs, const char *rhs){
    while(*lhs && *rhs){
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
