#include "string.h"

size_t ustrlen(const char* str)
{
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

int ustrcmp(const char *lhs, const char *rhs){
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

void int64_to_str_hexa(char* out, uint64_t v){
    for(size_t i=16; i;){
        --i;
        out[i] = hexa_digit(v&0xf);
        v = v >> 4;
    }
}

void int32_to_str_hexa(char* out, uint32_t v){
    for(size_t i=8; i;){
        --i;
        out[i] = hexa_digit(v&0xf);
        v = v >> 4;
    }
}

void int8_to_str_hexa(char* out, uint8_t v){
    for(size_t i=2; i;){
        --i;
        out[i] = hexa_digit(v&0xf);
        v = v >> 4;
    }
}
