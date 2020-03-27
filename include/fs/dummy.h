#ifndef _H_DUMMY
#define _H_DUMMY

#include <stdint.h>
#include <kernel/file.h>

#define BUFSIZE   1024

// very basic file system for tests purposes
// a block is a file
// root is always 0

#define KD_FREE 0
#define KD_USED 1
#define KD_DIR  2

struct inode {
    uint16_t ino_kind;
    uint16_t ino_size;
} __attribute__((packed));

struct block {
    struct inode blk_ino;
    off_t        blk_pos;

    // if a block is a directory
    // then the content is a null terminated list of directory entries
    uint8_t  blk_content[BUFSIZE];
} __attribute__((packed));

struct super_block {
    uint16_t block_size;
    uint16_t inode_nb;
    uint8_t  padding[sizeof(struct block) - 4];
} __attribute__((packed));

typedef struct block dummy_block_t;

int dummy_mount(void *, struct mount_info*);

int dummy_load(struct mount_info*,
               const char *fname, struct stat *st, char **end);
int dummy_create(struct mount_info*, ino_t parent, char *fname);

int dummy_seek(struct mount_info*, ino_t ino, off_t pos);
int dummy_read(struct mount_info*, ino_t ino, void *buf, size_t len);
int dummy_write(struct mount_info*, ino_t ino, void *buf, size_t len);
struct dirent *dummy_readdir(struct dirent*);

#endif
