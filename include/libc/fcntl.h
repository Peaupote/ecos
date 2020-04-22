#ifndef _LIBC_FCNTL_H
#define _LIBC_FCNTL_H

#include <headers/proc.h>
#include <headers/file.h>

int open(const char* fname, int oflags, ...);

#endif
