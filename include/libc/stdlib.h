#ifndef _LIBC_STDLIB_H
#define _LIBC_STDLIB_H

#include <util/test.h>
#include <stddef.h>
#include <stdint.h>

void*  TEST_U(malloc)(size_t size);
void   TEST_U(free)(void* ptr);

#endif
