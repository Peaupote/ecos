#ifndef _H_PROC_FS
#define _H_PROC_FS

/**
 * Not communicating with physical memory of any kind
 * gives informations about the kernel to the user through simple files
 *
 * The filsystem is architectured as follow:
 * /proc
 *   - /proc/pid
 *     - /proc/pid/stat process status
 *     - /proc/pid/cwd
 *
 * Inodes numbers are allocated as follow
 * /proc          : 0
 * /proc/pid      : pid * (1 + NB_PROC_FILE)
 * /proc/pid/stat : pid * (1 + NB_PROC_FILE) + 1
 * /proc/pid/cwd  : pid * (1 + NB_PROC_FILE) + 2
 *
 */

#include <kernel/file.h>

#define NB_PROC_FILE   3
#define STAT_PROC_FILE 1
#define CWD_PROC_FILE  2

int   proc_mount(void*, struct mount_info*);
int   proc_load(struct mount_info*, const char *fname, struct stat *st, char **end);
int   proc_create(struct mount_info*, ino_t ino, char *fname);
int   proc_seek(struct mount_info*, ino_t ino, off_t pos);
int   proc_read(struct mount_info*, ino_t ino, void *buf, size_t len);
int   proc_write(struct mount_info*, ino_t ino, void *buf, size_t len);

struct dirent *proc_readdir(struct dirent*);

#endif
