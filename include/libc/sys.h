#ifndef _LIBC_SYS_H
#define _LIBC_SYS_H

#include <headers/proc.h>

//TODO: mv to correct header
//sys/wait.h
pid_t    wait(int* status);
pid_t    waitpid(pid_t cpid, int* status);

int      _setpriority(int prio);
int      _getpriority();
int      setpriority(int prio);
int      getpriority();

int      debug_block(int v);

// Appel avec privilège ring 1

void     r1_prires(uint16_t new_ring);

#endif
