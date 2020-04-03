#ifndef _LIBC_STDLIB_H
#define _LIBC_STDLIB_H

#include <stddef.h>
#include <stdint.h>

int atoi(const char *str);

void*    malloc(size_t size);
void     free(void* ptr);

void     exit(int status); //TODO: _exit()

#endif
