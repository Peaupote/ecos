#ifndef _H_USER
#define _H_USER

#include <stdint.h>

#if defined(__is_kernel)
typedef uint16_t uid_t;
typedef uint16_t gid_t;
#else
#include <sys/stat.h>
#endif

#endif
