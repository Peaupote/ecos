#ifndef _H_FILE
#define _H_FILE

#include <stddef.h>
#include <stdint.h>
#include <kernel/param.h>
#include <kernel/user.h>
#include <kernel/devices.h>

typedef void * block_t;

#define TYPE_FIFO 0x1000
#define TYPE_CHAR 0x2000
#define TYPE_DIR  0x4000
#define TYPE_BLK  0x6000
#define TYPE_REG  0x8000
#define TYPE_SYM  0xa000
#define TYPE_SOCK 0xc000

#if defined(__is_kernel)

typedef uint32_t  ino_t;
typedef uint16_t mode_t;
typedef uint16_t nlink_t;
typedef uint64_t off_t;
typedef uint32_t blksize_t;
typedef uint32_t blkcnt_t;

// from man 2 stat
struct stat {
    dev_t     st_dev;         /* ID of device containing file */
    ino_t     st_ino;         /* Inode number */
    mode_t    st_mode;        /* File type and mode */
    nlink_t   st_nlink;       /* Number of hard links */
    uid_t     st_uid;         /* User ID of owner */
    gid_t     st_gid;         /* Group ID of owner */
    off_t     st_size;        /* Total size, in bytes */
    blksize_t st_blksize;     /* Block size for filesystem I/O */
    blkcnt_t  st_blocks;      /* Number of 512B blocks allocated */

    uint32_t st_ctime;
    uint32_t st_mtime;
};

#else
#include <stdio.h>
#include <sys/stat.h>
#endif

typedef struct vfile {
    struct stat vf_stat; // information about the file
    uint8_t     vf_cnt;  // nb of channel pointing at the file
} vfile_t;

#define DIRENT_OFF 8
struct dirent {
    ino_t    d_ino;
    uint16_t d_rec_len;
    uint8_t  d_name_len;
    uint8_t  d_file_type;
    char     d_name[]; // at most 255 bytes len
};

// File system table
#define NFST 3
#define DUMMY_FS 0
#define PROC_FS  1
#define EXT2_FS  2

/**
 * First parameter is a pointer to the beginning of the partition
 * mount fills the mount_info structure
 * return 0 if fails 1 otherwise
 */
typedef int (fs_mnt_t)(void*, struct mount_info*);

/**
 * Look for file named fname in directory with specified inode
 * return the file's inode if exists 0 otherwise
 */
typedef uint32_t (fs_lookup_t)(const char *fname, ino_t parent,
                               struct mount_info *info);

/**
 * Fill the given stat structure with corresponding information
 * return < 0 if fail
 */
typedef int (fs_stat_t)(ino_t ino, struct stat*, struct mount_info *info);

/**
 * Create a new empty file or a directory (containing '.' and '..')
 * with given name as child of parent
 * fail (returns 0) if parent is not a directory
 * returns inode of new file
 */
typedef uint32_t (fs_create_t)(ino_t parent, const char *fname, uint16_t type,
                              struct mount_info *info);

/**
 * Read/Write in file specified by given ino
 * return the number of char read or < 0 if error
 */
typedef int (fs_rdwr_t)(ino_t, void*, off_t, size_t, struct mount_info*);

/**
 * Returns a pointer to the first element in the directory
 */
typedef struct dirent * (fs_opendir)(ino_t, struct mount_info*);

typedef struct dirent *(fs_readdir_t)(struct dirent*);

struct fs {
    char           fs_name[4];
    fs_mnt_t      *fs_mnt;
    fs_lookup_t   *fs_lookup;
    fs_stat_t     *fs_stat;
    fs_rdwr_t     *fs_read;
    fs_rdwr_t     *fs_write;
    fs_create_t   *fs_touch;
    fs_create_t   *fs_mkdir;
    fs_opendir    *fs_opendir;
    fs_readdir_t  *fs_readdir;
} fst [NFST];

void  vfs_init();
int vfs_mount(const char *path, uint8_t fs, void *partition);

vfile_t *vfs_pipe();
vfile_t *vfs_load(const char *path, uint32_t create);
int      vfs_close(vfile_t *vfile);
int      vfs_read(vfile_t *vfile, void *buf, off_t pos, size_t len);
int      vfs_write(vfile_t *vfile, void *buf, off_t pos, size_t len);

vfile_t *vfs_touch(const char *parent, const char *fname, mode_t perm);
vfile_t *vfs_mkdir(const char *parent, const char *fname, mode_t perm);

struct dirent *vfs_opendir(const char *fname);

#endif
