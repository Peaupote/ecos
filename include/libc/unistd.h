#ifndef _LIBC_UNISTD_H
#define _LIBC_UNISTD_H

#include <stdint.h>
#include <stddef.h>

#include <headers/unistd.h>
#include <headers/proc.h>
#include <headers/file.h>
#include <headers/types.h>

int      brk(void* addr);
void*    sbrk(intptr_t increment);

uint64_t sleep(uint64_t);

ssize_t  write(int fd, const void *s, size_t len);
ssize_t  read(int fd, void *buf, size_t len);
int      close(int fd);
int      dup(int fd);
int      dup2(int fd1, int fd2);
int      pipe(int fds[2]);
off_t    lseek(int fd, off_t offset);

pid_t    fork(void);
pid_t    getpid(void);
pid_t    getppid(void);
int      execve(const char *fname, const char **argv, const char **env);

#endif
