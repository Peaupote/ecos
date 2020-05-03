#ifndef _K_TYPES_H
#define _K_TYPES_H

#ifdef  __use_host_libc
#include <stdio.h>
#else
typedef int ssize_t;
typedef long long unsigned usecond_t;
#endif

#endif
