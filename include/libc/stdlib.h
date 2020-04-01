#ifndef _LIBC_STDLIB_H
#define _LIBC_STDLIB_H

#include <stddef.h>
#include <stdint.h>

void*    malloc(size_t size);
void     free(void* ptr);

void     exit(int status); //TODO: _exit()

#endif
