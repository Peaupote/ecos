#ifndef _LIBC_SYS_H
#define _LIBC_SYS_H

#include <headers/proc.h>
#include <headers/file.h>

void     _libc_init();

//TODO: mv to correct header
//sys/wait.h
pid_t    wait(int* status);
pid_t    waitpid(pid_t cpid, int* status);

int      _setpriority(int prio);
int      _getpriority();
int      setpriority(int prio);
int      getpriority();

int      debug_block(int v);

// TODO: mv to correct header
// sys/stat.h

int fstat(int fd, struct stat *st);
int stat(const char *fname, struct stat *st);

// Appel avec privilège ring 1

void     r1_prires(uint16_t new_ring);

#endif
