#ifndef _H_SYS
#define _H_SYS

#include <headers/sys.h>

#ifndef ASM_FILE

#include <headers/types.h>

#include "proc.h"

uint64_t sys_sleep(uint64_t);
void     lookup_end_sleep(void);

pid_t    sys_wait(int* status);
pid_t    sys_waitpid(pid_t cpid, int* status);
pid_t    sys_fork(void);
void     sys_exit(int status);
pid_t    sys_getpid(void);
pid_t    sys_getppid(void);

int      sys_open(const char* fname, enum chann_mode mode);
int      sys_close(int fd);
int      sys_dup(int fd);
int      sys_pipe(int fds[2]);
ssize_t  sys_write(int fd, uint8_t *s, size_t len);
ssize_t  sys_read(int fd, uint8_t *buf, size_t len);
off_t    sys_lseek(int fd, off_t offset);

int      sys_execve(reg_t fname, reg_t argv, reg_t env);
int      sys_setpriority(int prio);
int      sys_getpriority();

int      sys_debug_block(int v);

uint64_t invalid_syscall();

void     prires_proc_struct(uint16_t new_ring);

#endif

#endif
