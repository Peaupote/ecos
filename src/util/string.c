#include <util/string.h>

size_t ustrlen(const char* str)
{
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

int ustrcmp(const char *lhs, const char *rhs) {
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

int is_prefix(const char *s, const char *p) {
    while (*p && *p++ == *s++);
    return *p;
}

void int64_to_str_hexa(char* out, uint64_t v) {
    for(size_t i=16; i;){
        --i;
        out[i] = hexa_digit(v&0xf);
        v = v >> 4;
    }
}

void int32_to_str_hexa(char* out, uint32_t v) {
    for(size_t i=8; i;){
        --i;
        out[i] = hexa_digit(v&0xf);
        v = v >> 4;
    }
}

void int8_to_str_hexa(char* out, uint8_t v) {
    for(size_t i=2; i;){
        --i;
        out[i] = hexa_digit(v&0xf);
        v = v >> 4;
    }
}

uint8_t u_hexa_digit_of_char_tb0[16] =
    {0x16,0x16,0x16,0x00, 0x0f,0x16,0x0f,0x16,
     0x16,0x16,0x16,0x16, 0x16,0x16,0x16,0x16};
uint8_t u_hexa_digit_of_char_tb1[16 * 3 - 9 - 1] =
    {0,1,2,3, 4,5,6,7, 8,9,~0,~0,   ~0,~0,~0,
     ~0,10,11,12, 13,14,15,~0, ~0,~0,~0,~0, ~0,~0,~0,~0,
     ~0,~0,~0,~0, ~0,~0,~0};

uint64_t int64_of_str_hexa(const char src[]) {
    uint64_t rt = 0;
    for(uint8_t d = hexa_digit_of_char(*src); d != (uint8_t)~0;
            d = hexa_digit_of_char(* ++src))
        rt = (rt << 4) + d;
    return rt;
}

size_t str_find_first(const char* s, char trg) {
    for (size_t i = 0; i != '\0'; ++i)
        if(s[i] == trg)
            return i;
    return ~0;
}
