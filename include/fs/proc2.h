#ifndef _H_PROC2_FS
#define _H_PROC2_FS

/**
 * Not communicating with physical memory of any kind
 * gives informations about the kernel to the user through simple files
 *
 * The filsystem is architectured as follow:
 * /proc (ROOT_INO)
 *   - /proc/tty
 *   - /proc/pipes
 *   - /proc/pid
 *     - /proc/pid/stat process status
 *     - /proc/pid/cwd
 *     - /proc/pid/fd   files opened by process
 *
 */

#include <kernel/proc.h>
#include <kernel/file.h>
#include <libc/string.h>

// ino 0 means end of dirent list
#define PROC_NULL_INO 0
#define PROC_ROOT_INO 1

int      fs_proc_mount(void*, struct mount_info *info);
uint32_t fs_proc_lookup(const char *fname, ino_t parent, struct mount_info*);
int      fs_proc_stat(ino_t ino, struct stat *st, struct mount_info*);
int      fs_proc_read(ino_t ino, void*, off_t, size_t, struct mount_info*);
int      fs_proc_write(ino_t ino, void*, off_t, size_t, struct mount_info*);
struct dirent_it* fs_proc_opendir(ino_t, struct dirent_it*,
			char name_buf[], struct mount_info *);
struct dirent_it* fs_proc_readdir(struct dirent_it*, char name_buf[]);
ino_t    fs_proc_readsymlink(ino_t, char*, struct mount_info *);


uint32_t fs_proc_alloc_pipe(ino_t parent, const char *fname, uint16_t type);
uint32_t fs_proc_alloc_std_streams(pid_t pid);


uint32_t fs_proc_mkdir(ino_t, const char*, uint16_t, struct mount_info*);
ino_t    fs_proc_rm(ino_t ino, struct mount_info *);
ino_t    fs_proc_rmdir(ino_t ino, struct mount_info *);
ino_t    fs_proc_destroy_dirent(ino_t p, ino_t ino, struct mount_info*);
uint32_t fs_proc_touch(ino_t, const char*, uint16_t, struct mount_info*);

#endif
