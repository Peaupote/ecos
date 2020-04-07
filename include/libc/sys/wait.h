#ifndef _LIBC_SYS_WAIT_H
#define _LIBC_SYS_WAIT_H

#include <headers/proc.h>

pid_t    wait(int* status);
pid_t    waitpid(pid_t cpid, int* status);

#endif
