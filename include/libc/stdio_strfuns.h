#ifndef _LIBC_STDIO_STRFUN_H
#define _LIBC_STDIO_STRFUN_H

#include <headers/types.h>

#include <stddef.h>
#include <stdarg.h>

typedef void (*string_writer)(void*, const char*);
//str[sz] doit être accessible mais pas nécessairement égal à '\0'
typedef void (*stringl_writer)(void*, const char*, size_t);

typedef struct {
	ssize_t (* read)(void*, char*, size_t);
	void    (*unget)(void*);
} string_reader;

int fpprintf(stringl_writer w, void* wi, const char* fmt, va_list ps);

ssize_t fpscanff(string_reader r, void* ri, const char* fmt, va_list ps);

#endif
