#ifndef _H_LIBC_STDIO
#define _H_LIBC_STDIO

#include <stddef.h>
#include <stdarg.h>

typedef void (*string_writer)(void*, const char*);
//str[sz] doit être accessible mais pas nécessairement égal à '\0'
typedef void (*stringl_writer)(void*, const char*, size_t);

int fpprintf(stringl_writer w, void* wi, const char* fmt, va_list ps);

int putchar(int c);

#if defined(__is_libc)
int printf(const char *fmt, ...);

int puts(const char *s);

void abort(void);
void exit(int status);
#endif


#endif
