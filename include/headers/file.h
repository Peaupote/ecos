#ifndef _HD_FILE_H
#define _HD_FILE_H

#include <stddef.h>
#include <stdint.h>

#include "user.h"
#include "devices.h"

typedef void * block_t;
typedef uint64_t off_t;

typedef int32_t  ino_t;
typedef uint32_t mode_t;
typedef uint16_t nlink_t;
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

#endif
