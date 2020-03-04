#ifndef _H_SYS
#define _H_SYS

#define SYS_SLEEP 0

#ifndef ASSEMBLY

#include "proc.h"
#include "syscalls/sleep.h"

uint32_t sleep(uint32_t time);
pid_t wait();
pid_t fork();
void  kexit(int status);

int open(char *fname, enum chann_mode);
int close(int fildes);
int write(int filedes, const void *buf, size_t len);
int read (int filedes, const void *buf, size_t len);


void syscall_hdl(void);

#endif

#endif
