#ifndef _HD_SYS_H
#define _HD_SYS_H

#define NSYSCALL 24

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
#define SYS_EXECVE  14
#define SYS_FSTAT   15
#define SYS_SETPRIO 16
#define SYS_GETPRIO 17
#define SYS_SBRK    18
#define SYS_SIGHND  19
#define SYS_SIGRT   20
#define SYS_SIGNAL  21
#define SYS_KILL    22
#define SYS_DEBUG_BLOCK 23

#define SYS_R1J_PRIRES   0
#define SYS_R1C_TTYOWN   0

#endif
