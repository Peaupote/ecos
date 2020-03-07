#ifndef _H_SYS
#define _H_SYS

#define NSYSCALL 3

#define SYS_SLEEP 0
#define SYS_FORK  1
#define SYS_EXIT  2

#ifndef ASSEMBLY

#include "proc.h"
#include "syscalls/sleep.h"

void sleep(void);
void wait(void);
void fork(void);
void kexit(void);

/* int open(char *fname, enum chann_mode); */
/* int close(int fildes); */
/* int write(int filedes, const void *buf, size_t len); */
/* int read (int filedes, const void *buf, size_t len); */

#endif

#endif
