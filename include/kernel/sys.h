#ifndef _H_SYS
#define _H_SYS

#define NSYSCALL 14

#define SYS_SLEEP    0
#define SYS_FORK     1
#define SYS_EXIT     2
#define SYS_WAIT     3
#define SYS_WAITPID  4
#define SYS_GETPID   5
#define SYS_GETPPID  6
#define SYS_OPEN     7
#define SYS_CLOSE    8
#define SYS_DUP      9
#define SYS_PIPE    10
#define SYS_WRITE   11
#define SYS_READ    12
#define SYS_LSEEK   13

#ifndef ASM_FILE

#include "proc.h"

void sleep(void);
void lookup_end_sleep(void);

void wait(void);
void fork(void);
void kexit(void);
void getpid(void);
void getppid(void);
void waitpid(void);

void open(void);
void close(void);
void dup(void);
void pipe(void);
void write(void);
void read(void);
void lseek(void);

void invalid_syscall();

#endif

#endif
