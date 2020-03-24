#ifndef _H_FILE
#define _H_FILE

#include <stddef.h>
#include <stdint.h>
#include <kernel/param.h>
#include <kernel/user.h>
#include <kernel/devices.h>

typedef int32_t  ino_t;
typedef uint32_t mode_t;
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
};

typedef struct vfile {
    struct stat vf_stat; // information about the file
    uint8_t     vf_cnt;  // nb of channel pointing at the file
    uint8_t     vf_mnt;
} vfile_t;

struct dirent {
    ino_t ino;
    char  fname[255];
};

// File system table
#define NFST 2
#define DUMMY_FS 0
#define PROC_FS  1

struct fs {
    char             fs_name[4];
    void          *(*fs_mnt)();
    int            (*fs_load)(void*, char *fname, struct stat *st, char**);
    int            (*fs_create)(void*, ino_t, char*);
    int            (*fs_seek)(void*, ino_t, off_t);
    int            (*fs_read)(void*, ino_t, void*, size_t);
    int            (*fs_write)(void*, ino_t, void*, size_t);
    struct dirent *(*fs_readdir)(struct dirent*);
} fst [NFST];

void     vfs_init();
vfile_t *vfs_load(const char *path, uint32_t create);
int      vfs_seek(vfile_t *vfile, off_t pos);
int      vfs_read(vfile_t *vfile, void *buf, size_t len);
int      vfs_write(vfile_t *vfile, void *buf, size_t len);

#endif
