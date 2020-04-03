#ifndef _H_PROC_SPECIAL
#define _H_PROC_SPECIAL

#include <headers/file.h>
#include <headers/proc.h>
#include <kernel/file.h>
#include <kernel/proc.h>

#define SPEC_PID   0
#define SPEC_READ  1
#define SPEC_STAT  2

typedef int (proc_st_t)(pid_t pid, ino_t ino, struct stat *st);
typedef int (proc_op_t)(pid_t pid, void *buf, off_t pos, size_t len);

// stat file contains...
int stat_read(pid_t pid, void *buf, off_t pos, size_t len);
int stat_stat(pid_t, ino_t, struct stat *st);

// cmd file
int cmd_read(pid_t pid, void *buf, off_t pos, size_t len);
int cmd_stat(pid_t, ino_t, struct stat *st);

ino_t alloc_special_file(pid_t pid, proc_op_t *read, proc_st_t *stat);

#endif
