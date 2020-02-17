#ifndef _STRING_H
#define _STRING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*string_writer)(void*, const char*);

size_t ustrlen(const char* str);
int ustrcmp(const char *lhs, const char *rhs);

void int64_to_str_hexa(char* out, uint64_t v);
void int32_to_str_hexa(char* out, uint32_t v);
void int8_to_str_hexa(char* out, uint8_t v);

inline void ptr_to_str(char *out, void *ptr){
#ifdef __i686__
    int32_to_str_hexa(out, (uint32_t)ptr);
#else
    int64_to_str_hexa(out, (uint64_t)ptr);
#endif
}

static inline char hexa_digit(uint8_t p){
    return (p<10 ? '0': ('A'-10)) + p;
}

#endif
