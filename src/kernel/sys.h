#ifndef _H_SYS
#define _H_SYS

#define NSYSCALL 7

#define SYS_SLEEP   0
#define SYS_FORK    1
#define SYS_EXIT    2
#define SYS_WAIT    3
#define SYS_WAITPID 4
#define SYS_GETPID  5
#define SYS_GETPPID 6


#ifndef ASSEMBLY

#include "proc.h"
#include "syscalls/sleep.h"

void sleep(void);
void wait(void);
void fork(void);
void kexit(void);
void getpid(void);
void getppid(void);
void wait(void);

/* int open(char *fname, enum chann_mode); */
/* int close(int fildes); */
/* int write(int filedes, const void *buf, size_t len); */
/* int read (int filedes, const void *buf, size_t len); */

void invalid_syscall();

#endif

#endif
