#ifndef _LIBC_SIGNAL_H
#define _LIBC_SIGNAL_H

#include <headers/signal.h>
#include <headers/proc.h>

sighandler_t signal(int signum, sighandler_t hnd);
int kill(pid_t pid, int signum);

#endif
