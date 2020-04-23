#ifndef _H_SYS
#define _H_SYS

#include <headers/sys.h>

#ifndef ASM_FILE

#include <headers/types.h>
#include <headers/signal.h>
#include <libc/errno.h>

#include "proc.h"

static inline void set_proc_errno(proc_t *p, int e) {
    if (p->p_errno) *p->p_errno = e;
}

static inline void set_errno(int e) {
    set_proc_errno(state.st_proc + state.st_curr_pid, e);
}

uint64_t sys_sleep(uint64_t);
void     lookup_end_sleep(void);

pid_t    sys_wait(int* status);
pid_t    sys_waitpid(pid_t cpid, int* status);
pid_t    sys_fork(void);

__attribute__ ((noreturn))
void     sys_exit(int status);

pid_t    sys_getpid(void);
pid_t    sys_getppid(void);

int      sys_open(const char* fname, int oflags, int perms);
int      sys_close(int fd);
int      sys_dup(int fd);
int      sys_pipe(int fds[2]);
ssize_t  sys_write(int fd, uint8_t *s, size_t len);
ssize_t  sys_read(int fd, uint8_t *buf, size_t len);
off_t    sys_lseek(int fd, off_t offset, int whence);
int      mkdir(const char *fname, mode_t mode);

int      sys_execve(reg_t fname, reg_t argv, reg_t env);

int      sys_fstat(int fd, struct stat *st);

int      sys_setpriority(int prio);
int      sys_getpriority();

void*    sys_sbrk(intptr_t inc);

void     sys_sigsethnd(sighandler_t);

__attribute__ ((noreturn))
void     sys_sigreturn();

// 0 <= sigid < SIG_COUNT
// hnd[0]: ign, hnd[1]: dfl
uint8_t  sys_signal(int sigid, uint8_t hnd);
int      sys_kill(pid_t pid, int signum);

int      sys_debug_block(int v);

uint64_t invalid_syscall();

void     prires_proc_struct(uint16_t new_ring);

#endif

#endif
