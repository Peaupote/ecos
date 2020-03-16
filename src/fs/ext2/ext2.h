#ifndef _H_EXT2
#define _H_EXT2

#include <stdint.h>

typedef int32_t  time_t;

// file system state
#define FS_STAT_CLEAN 1
#define FS_STAT_ERROR 2

// error handling method
#define FS_ERR_IGNORE  1
#define FS_ERR_REMOUNT 2
#define FS_ERR_PANIC   3

// os id by which filesystem was created
#define FS_ID_LINUX   0
#define FS_ID_GNU     1
#define FS_ID_MASIX   2
#define FS_ID_FREEBSD 3
#define FS_ID_OTHER   4

// super block
struct superblock {
    uint32_t nb_inodes;
    uint32_t nb_blocks;
    uint32_t nb_res_blocks;
    uint32_t nb_free_blocs;
    uint32_t nb_free_inodes;
    uint32_t superblock_nbblock;

    // log_2(blocksize) - 10
    // the number to shift 1,024 to the left by to obtain the block size
    uint32_t shift_block_size;

    // log_2(fragmentsize) - 10
    // the number to shift 1,024 to the left by to obtain the fragment size
    uint32_t shift_fragment_size;

    // number of blocks in each block group
    uint32_t bg_size;

    // number of fragment in each block group
    uint32_t frag_size;

    // number of inodes in each block group
    uint32_t inode_size;

    time_t  last_mount;
    time_t  last_write;

    // number of mount since last consitency check
    uint16_t nb_mount;
    // max number of mount allowed before next check
    uint16_t max_nb_mount;

    // ext2 signature
    uint16_t ext2_sig;

    uint16_t fs_state;
    uint16_t err_handling;
    uint16_t minor;

    time_t last_check;
    time_t interval_check;

    uint32_t creator_os_id;
    uint32_t major;

    uint32_t user;
    uint32_t group;
} __attribute__((packed));

// block group descriptor
struct bgd {
    uint32_t block_addr_bitmap;
    uint32_t block_inode_bitmap;
    uint32_t start_inode_stable;
    uint16_t nb_free_block;
    uint16_t nb_free_inodes;
    uint16_t nb_dir;
    uint8_t  padding[14];
} __attribute__((packed));

struct inode {
    uint16_t in_type;
    uint16_t in_uid;
    uint32_t in_size_low;

    time_t   in_last_time;
    time_t   in_create_time;
    time_t   in_modif_time;
    time_t   in_del_time;

    uint16_t in_gid;
    uint16_t in_hard;
    uint32_t in_dsksec;
    uint32_t in_flags;
    uint32_t in_os;

    // direct block pointer
    uint32_t in_dbp[12];
    // singely indirect block pointer
    uint32_t in_sibp;
    // doubly indirect block pointer
    uint32_t in_dibp;
    // triply indirect block pointer
    uint32_t in_tibp;

    uint32_t in_gen;
    uint32_t in_acl;
    uint32_t in_size_high;

};

#endif
