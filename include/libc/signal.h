#ifndef _LIBC_SIGNAL_H
#define _LIBC_SIGNAL_H

#include <headers/signal.h>

sighandler_t signal(int signum, sighandler_t hnd);

#endif
