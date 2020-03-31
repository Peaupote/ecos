#ifndef _LIBC_SYS_H
#define _LIBC_SYS_H

#include <headers/proc.h>
#include <headers/file.h>

//TODO: mv to correct header

uint64_t sleep(uint64_t);

pid_t    wait(int* status);

pid_t    waitpid(int* status, pid_t);

pid_t    fork(void);

void     exit(int status);

pid_t    getpid(void);

pid_t    getppid(void);

int      open(const char* fname, enum chann_mode mode);

int      close(int fd);

int      dup(int fd);

int      pipe(int fds[2]);

int      write(int fd, const void *s, size_t len);

int      read(int fd, void *buf, size_t len);

off_t    lseek(int fd, off_t offset);

int      execve(const char *fname, const char **argv, const char **env);

// Appel avec privilège ring 1

void     r1_prires(uint16_t new_ring);

#endif
