#ifndef _H_LIBC_SYS_STAT
#define _H_LIBC_SYS_STAT

#include <headers/file.h>

int fstat(int fd, struct stat *st);
int stat(const char *fname, struct stat *st);
int lstat(const char *fname, struct stat *st);
int mkdir(const char *fname, mode_t mode);

#endif
