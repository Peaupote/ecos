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
 *     - /proc/pid/fd   files opened by process
 *
 */

#include <kernel/proc.h>
#include <kernel/file.h>
#include <libc/string.h>

#define PROC_NBLOCKS    256
#define PROC_BLOCK_SIZE 1024
#define PROC_BITMAP_SIZE (PROC_NBLOCKS >> 3)

uint8_t proc_block_bitmap[PROC_BITMAP_SIZE];

struct proc_block {
    char    content[PROC_BLOCK_SIZE];
} proc_blocks[PROC_NBLOCKS];

static inline uint32_t proc_free_block() {
    size_t b;
    for (b = 0; b < PROC_NBLOCKS; b++) {
        if (!(proc_block_bitmap[b >> 3] & (1 << (b&7)))) break;
    }

    return b;
}

// ino 0 means end of dirent list
#define PROC_NULL_INO 0
#define PROC_ROOT_INO 1

#define PROC_NBLOCK 15
#define PROC_NINODES (NPROC * 2)
struct proc_inode {
    struct stat st;

    // pointers to auxiliary memory if necessary
    uint64_t in_block[PROC_NBLOCK];
} proc_inodes[PROC_NINODES];

static inline uint32_t proc_free_inode() {
    uint32_t ino;
    for (ino = 2; ino < PROC_NINODES; ino++) {
        if (!proc_inodes[ino].st.st_nlink) break;
    }

    return ino == PROC_NINODES ? 0 : ino;
}

int proc_mount(void*, struct mount_info *info);
uint32_t proc_lookup(const char *fname, ino_t parent, struct mount_info*);
int      proc_stat(ino_t ino, struct stat *st, struct mount_info*);
int      proc_read(ino_t ino, void*, off_t, size_t, struct mount_info*);
int      proc_write(ino_t ino, void*, off_t, size_t, struct mount_info*);
uint32_t proc_touch(ino_t, const char*, uint16_t, struct mount_info*);
uint32_t proc_mkdir(ino_t, const char*, uint16_t, struct mount_info*);

struct dirent *proc_opendir(ino_t, struct mount_info *);
struct dirent *proc_readdir(struct dirent*);

ino_t proc_readsymlink(ino_t, char*, struct mount_info *);

ino_t proc_rm(ino_t ino, struct mount_info *);
ino_t proc_rmdir(ino_t ino, struct mount_info *);
ino_t proc_destroy_dirent(ino_t p, ino_t ino, struct mount_info*);

uint32_t proc_alloc_pipe(ino_t parent, const char *fname, uint16_t type);

uint32_t proc_create(pid_t pid);
uint32_t proc_alloc_std_streams(pid_t pid);
uint32_t proc_exit(pid_t pid);

static inline void
proc_fill_dirent(struct dirent *dir, uint32_t ino, const char *fname) {
    dir->d_ino       = ino;
    dir->d_file_type = proc_inodes[ino].st.st_mode;
    dir->d_name_len  = strlen(fname);
    dir->d_rec_len   = DIRENT_OFF + dir->d_name_len;
    memcpy(dir->d_name, fname, dir->d_name_len);
}

uint32_t proc_add_dirent(struct proc_inode *, int32_t, const char*);
uint32_t proc_init_dir(uint32_t ino, uint32_t parent, uint16_t type);
#endif
