#ifndef _HD_SYS_H
#define _HD_SYS_H

#define NSYSCALL 28

#define SYS_SLEEP        0
#define SYS_FORK         1
#define SYS_EXIT         2
#define SYS_WAIT         3
#define SYS_WAITPID      4
#define SYS_GETPID       5
#define SYS_GETPPID      6
#define SYS_OPEN         7
#define SYS_CLOSE        8
#define SYS_DUP          9
#define SYS_DUP2        10
#define SYS_PIPE        11
#define SYS_WRITE       12
#define SYS_READ        13
#define SYS_LSEEK       14
#define SYS_EXECVE      15
#define SYS_FSTAT       16
#define SYS_MKDIR       17
#define SYS_CHDIR       18
#define SYS_SETPRIO     19
#define SYS_GETPRIO     20
#define SYS_SBRK        21
#define SYS_SIGHND      22
#define SYS_SIGRT       23
#define SYS_SIGNAL      24
#define SYS_KILL        25
#define SYS_SETERRNO    26
#define SYS_DEBUG_BLOCK 27

#define SYS_R1J_PRIRES   0
#define SYS_R1C_TTYOWN   0

#endif
