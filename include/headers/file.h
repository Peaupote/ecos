#ifndef _HD_FILE_H
#define _HD_FILE_H

#include <stddef.h>
#include <stdint.h>

#include "user.h"
#include "devices.h"

typedef void * block_t;

#define FTYPE_MASK 0xf000
#define TYPE_FIFO  0x1000
#define TYPE_CHAR  0x2000
#define TYPE_DIR   0x4000
#define TYPE_BLK   0x6000
#define TYPE_REG   0x8000
#define TYPE_SYM   0xa000
#define TYPE_SOCK  0xc000

typedef int64_t  off_t;
typedef uint32_t ino_t;
typedef uint16_t mode_t;
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
    size_t    st_size;        /* Total size, in bytes */
    blksize_t st_blksize;     /* Block size for filesystem I/O */
    blkcnt_t  st_blocks;      /* Number of 512B blocks allocated */

    uint32_t st_ctime;
    uint32_t st_mtime;
};

#define DIRENT_OFF 8
struct ext2_dir_entry {
    uint32_t d_ino;
    uint16_t d_rec_len;
    uint8_t  d_name_len;
    uint8_t  d_file_type;
    char     d_name[]; // at most 255 bytes len
};

struct dirent {
	ino_t    d_ino;
    uint16_t d_rec_len;
	uint8_t  d_name_len;
	uint8_t  d_file_type;
	char*    d_name; // at most 255 bytes len
};
struct dirent_it {
	struct dirent cnt;
	// espace permettant de stocker des données spécifique
	// au système de fichier
	char dt[16] __attribute__ ((aligned (16)));    
};

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#endif
