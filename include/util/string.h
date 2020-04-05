#ifndef _STRING_H
#define _STRING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

size_t ustrlen(const char* str);
int ustrcmp(const char *lhs, const char *rhs);
int is_prefix(const char *s, const char *p);

size_t str_find_first(const char* s, char trg);

void int64_to_str_hexa(char* out, uint64_t v);
void int32_to_str_hexa(char* out, uint32_t v);
void int8_to_str_hexa(char* out, uint8_t v);
static inline void ptr_to_str(char *out, void *ptr){
#ifdef __i686__
    int32_to_str_hexa(out, (uint32_t)ptr);
#else
    int64_to_str_hexa(out, (uint64_t)ptr);
#endif
}

uint64_t int64_of_str_hexa(const char src[]);

static inline char hexa_digit(uint8_t p){
    return (p<10 ? '0': ('A'-10)) + p;
}
extern uint8_t u_hexa_digit_of_char_tb0[16];
extern uint8_t u_hexa_digit_of_char_tb1[16 * 3 - 9 - 1];
static inline uint8_t hexa_digit_of_char(char c){
    return u_hexa_digit_of_char_tb1[
            u_hexa_digit_of_char_tb0[(c>>4)&0xf] + (c&0xf)];
}

#endif
