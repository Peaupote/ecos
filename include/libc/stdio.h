#ifndef _H_LIBC_STDIO
#define _H_LIBC_STDIO

#include <libc/stdio_strfuns.h>

#ifndef __is_kernel

int  putchar(int c);

int  puts(const char *s);

int  printf(const char *fmt, ...);

int  scanf(const char *format, ...);

#endif

int sprintf(char *str, const char *format, ...);

int sscanf(const char *str, const char *format, ...);

#endif
