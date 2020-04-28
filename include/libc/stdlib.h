#ifndef _LIBC_STDLIB_H
#define _LIBC_STDLIB_H

#include <stddef.h>
#include <stdint.h>

#define NENV 256
char *_env[NENV];
char *getenv(const char *name);
int   setenv(const char *name, const char *value, int overwrite);

int atoi(const char *str);

void*    calloc(size_t nmemb, size_t size);
void*    malloc(size_t size);
void*    realloc(void* ptr, size_t size);
void     free(void* ptr);

__attribute__((noreturn))
void     exit(int status);

#endif
