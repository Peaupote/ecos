#ifndef _LIBC_UNISTD_H
#define _LIBC_UNISTD_H

#include <stdint.h>

int      brk(void* addr);
void*    sbrk(intptr_t increment);

#endif
