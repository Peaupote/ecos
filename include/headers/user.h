#ifndef _HD_USER_H
#define _HD_USER_H

#include <stdint.h>

#if defined(__is_test)
#else
typedef uint16_t uid_t;
typedef uint16_t gid_t;
#endif

#endif
