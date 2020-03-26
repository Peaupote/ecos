#ifndef _H_SYS
#define _H_SYS

#define NSYSCALL 15

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

#define SYS_R1_PRIRES 0
#define SYS_R1_IDLE   1

#ifndef ASM_FILE

#include "proc.h"

uint64_t sleep(uint64_t);
void     lookup_end_sleep(void);

pid_t    wait(void);
pid_t    fork(void);
void     kexit(int status);
pid_t    getpid(void);
pid_t    getppid(void);
pid_t    waitpid(pid_t);

int      open(const char* fname, enum chann_mode mode);
int      close(int fd);
int      dup(int fd);
int      pipe(int fds[2]);
int      write(int fd, uint8_t *s, size_t len);
int      read(int fd, uint8_t *buf, size_t len);
off_t    lseek(int fd, off_t offset);

int      execve(const char *fname, const char **argv, const char **env);

uint64_t invalid_syscall();

void     prires_proc_struct(void);

#endif

#endif
