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

int      usleep(usecond_t usec);
unsigned int sleep(unsigned int sec);

int      pause();

ssize_t  write(int fd, const void *s, size_t len);
ssize_t  read(int fd, void *buf, size_t len);
int      close(int fd);
int      dup(int fd);
int      dup2(int fd1, int fd2);
int      pipe(int fds[2]);
off_t    lseek(int fd, off_t offset, int whence);

int      chdir(const char *fname);

int      link(const char *path1, const char *path2);
int      symlink(const char *path1, const char *path2);
int      readlink(const char *path, char *buf, size_t len);

pid_t    fork(void);
pid_t    getpid(void);
pid_t    getppid(void);

int      execv(const char *fname, const char **argv);
int      execvp(const char *fname, const char **argv);
int      execve(const char *fname, const char **argv, const char **env);
int      execvpe(const char *fname, const char** argv, const char **env);

#endif
