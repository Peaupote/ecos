#ifndef _HD_DEV_H
#define _HD_DEV_H

#include <stdint.h>

#if defined(__is_test)
#else
typedef uint32_t dev_t;
#endif
#endif
