#ifndef _H_LIBC_STDIO
#define _H_LIBC_STDIO

#include <libc/stdio_strfuns.h>

#ifndef __is_kernel

int  putchar(int c);

int  puts(const char *s);

int  fdprintf(int fd, const char *fmt, ...);

int  printf(const char *fmt, ...);

int  scanf(const char *format, ...);

void perror(const char *str);

#endif

int sprintf(char *str, const char *format, ...);

int sscanf(const char *str, const char *format, ...);

#endif
