#ifndef _H_EXT2
#define _H_EXT2

#include <stddef.h>
#include <stdint.h>
#include <kernel/file.h>

struct ext2_mount_info {
    // first section common to kernel mount_info
    struct ext2_superblock *sp;
    uint32_t                block_size;
    uint32_t                root_ino;

    // position of the first block group descriptor
    struct ext2_group_desc *bg;
    uint32_t                group_count;
    uint32_t                group_size;
};

typedef void * block_t;

// file system state
#define EXT2_STAT_CLEAN 1
#define EXT2_STAT_ERROR 2

// error handling method
#define EXT2_ERR_IGNORE  1
#define EXT2_ERR_REMOUNT 2
#define EXT2_ERR_PANIC   3

// os id by which filesystem was created
#define EXT2_OS_LINUX   0
#define EXT2_OS_GNU     1
#define EXT2_OS_MASIX   2
#define EXT2_OS_FREEBSD 3
#define EXT2_OS_OTHER   4

// optional feature flag
#define EXT2_FEATURE_PREALLOCATE 0x0001
#define EXT2_FEATURE_AFS         0x0002
#define EXT2_FEATURE_HAS_JOURNAL 0x0004
#define EXT2_FEATURE_EXT_ATTR    0x0008
#define EXT2_FEATURE_EXT_RESIZE  0x0010
#define EXT2_FEATURE_HASH_INDEX  0x0020

// required feature compat
#define EXT2_FEATURE_COMPRESSION 0x0001
#define EXT2_FEATURE_FILETYPE    0x0002
#define EXT2_FEATURE_REPLAY      0x0004
#define EXT2_FEATURE_JOURNAL_DEV 0x0008

// readonly flags
#define EXT2_FEATURE_SPARSE_SUPER 0x0001
#define EXT2_FEATURE_LARGE_FILE   0x0002
#define EXT2_FEATURE_BTREE_DIR    0x0004

// super block
struct ext2_superblock {
    uint32_t s_inodes_count; // total number of inodes
    uint32_t s_blocks_count; // total number of blocks
    uint32_t s_r_block_count; // reserved block count
    uint32_t s_free_block_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;

    // log_2(blocksize) - 10
    // the number to shift 1,024 to the left by to obtain the block size
    uint32_t s_log_block_size;

    // log_2(fragmentsize) - 10
    // the number to shift 1,024 to the left by to obtain the fragment size
    uint32_t s_log_frag_size;

    uint32_t s_blocks_per_group; // number of blocks in each block group
    uint32_t s_frags_per_group;  // number of fragment in each block group
    uint32_t s_inodes_per_group; // number of inodes in each block group

    uint32_t  s_mtime; // last mount time
    uint32_t  s_wtime; // last write time

    uint16_t s_mount_count; // number of mount since last consitency check
    uint16_t s_max_mount_count; // max number of mount allowed before next check

    uint16_t s_magic; // ext2 signature

    uint16_t s_state;           // file system state
    uint16_t s_err;             // behaviors when detecting errors
    uint16_t s_minor_rev_level; // minor revision level

    uint32_t s_lastcheck;
    uint32_t s_checkinterval;

    uint32_t s_creator_os;
    uint32_t s_rev_level; // revision level

    uint16_t s_def_user;  // default user id
    uint16_t s_def_group; // default group id

    // extended superblock fields

    uint32_t s_first_ino;      // first non reserved ino
    uint16_t s_inode_size;     // size of inode structure
    uint16_t s_block_group_nb; // block group nb of this super block

    uint32_t s_feature_compat;    // compatible feature set
    uint32_t s_feature_incompat;  // incompatible feature set
    uint32_t s_feature_ro_compat; // readonly compatible feature

    uint8_t  s_uuid[16];          // file system id
    char     s_volume_name[16];
    char     s_last_mounted[64]; // directory where last mounted

    uint32_t s_algo_usage_bitmap;   // for compression
    uint8_t  s_prealloc_blocks;     // number of blocks to preallocate for files
    uint8_t  s_prealloc_dir_blocks; // number of blocks to preallocate for directories

    uint16_t s_padding;

    // journaling
    uint8_t s_journal_uuid[16]; // same as above
    uint32_t s_journal_inum;    // inode number of the journal file
    uint32_t s_journal_dev;     // device number of journal file
    uint32_t s_last_orphan;     // start of list of inodes to delete
    uint8_t  s_reserved[788];   // supposed to be real data here
} __attribute__((packed));

static inline uint32_t ext2_block_size(struct ext2_superblock *sp) {
    return 1024 << sp->s_log_block_size;
}

static inline uint32_t ext2_frag_size(struct ext2_superblock *sp) {
    return 1024 << sp->s_log_frag_size;
}

static inline uint32_t ext2_group_count(struct ext2_superblock *sp) {
    if (sp->s_blocks_count <= sp->s_blocks_per_group) return 1;
    return sp->s_blocks_count / sp->s_blocks_per_group +
        (sp->s_blocks_count % sp->s_blocks_per_group != 0 ? 1 : 0);
}

static inline uint32_t ext2_group_count_inodes(struct ext2_superblock *sp) {
    return sp->s_inodes_count / sp->s_inodes_per_group +
        (sp->s_inodes_count % sp->s_inodes_per_group != 0 ? 1 : 0);
}

static inline block_t
ext2_get_block(uint32_t n, struct ext2_mount_info *info) {
    return (char*)info->sp + (n - 1) * info->block_size;
}

int is_superblock_sparse(uint32_t x);

// block group descriptor
struct ext2_group_desc {
    uint32_t g_block_bitmap;
    uint32_t g_inode_bitmap;
    uint32_t g_inode_table;
    uint16_t g_free_blocks_count;
    uint16_t g_free_inodes_count;
    uint16_t g_dir_count;
    uint16_t g_pad;
    uint32_t g_padding[3];
} __attribute__((packed));

// inodes

// inodes type and permissions
// permission classic unix format
#define EXT2_TYPE_FIFO 0x1000
#define EXT2_TYPE_CHAR 0x2000
#define EXT2_TYPE_DIR  0x4000
#define EXT2_TYPE_BLK  0x6000
#define EXT2_TYPE_REG  0x8000
#define EXT2_TYPE_SYM  0xa000
#define EXT2_TYPE_SOCK 0xc000

// inode flags
#define EXT2_INO_FLAG_SECURE_DEL  0x00000001
#define EXT2_INO_FLAG_KEEP_COPY   0x00000002
#define EXT2_INO_FLAG_COMPRESSION 0x00000004
#define EXT2_INO_FLAG_SYNC_UPDATE 0x00000008
#define EXT2_INO_FLAG_IMM_FILE    0x00000010
#define EXT2_INO_FLAG_APPEND_ONLY 0x00000020
#define EXT2_INO_FLAG_NOT_DUMP    0x00000040
#define EXT2_INO_FLAG_KEEP_UPDATE 0x00000080 // dont update update time
#define EXT2_INO_FLAG_HASH        0x00010000
#define EXT2_INO_FLAG_AFS_DIR     0x00020000
#define EXT2_INO_FLAG_JOURNAL     0x00040000

// reserved ino
#define EXT2_BAD_INO         1
#define EXT2_ROOT_INO        2
#define EXT2_ACL_IDX_INO     3
#define EXT2_ACL_DATA_INO    4
#define EXT2_BOOT_LOADER_INO 5
#define EXT2_UNDEL_DIR_INO   6

#define EXT2_DIRECT_BLOCK 12

struct ext2_inode {
    uint16_t in_type;
    uint16_t in_uid;
    uint32_t in_size;

    uint32_t in_atime;
    uint32_t in_ctime;
    uint32_t in_mtime;
    uint32_t in_dtime;

    uint16_t in_gid;
    uint16_t in_hard;
    uint32_t in_blocks;
    uint32_t in_flags;
    uint32_t in_os;

    // first EXT2_DIRECT_BLOCK are blocks
    // next one is 1 indirected
    // next one is 2 indirected
    // ..
    uint32_t in_block[EXT2_DIRECT_BLOCK + 3];

    // unused
    uint32_t in_gen;
    uint32_t in_file_acl;
    uint32_t in_dir_acl;

    uint32_t in_faddr;
    uint8_t  in_reserved[12];
};

static inline uint32_t
ext2_inode_block_group(uint32_t inode, struct ext2_superblock *sp) {
    return (inode - 1) / sp->s_inodes_per_group;
}

static inline uint32_t
ext2_inode_index(uint32_t inode, struct ext2_superblock *sp) {
    return (inode - 1) % sp->s_inodes_per_group;
}

static inline struct ext2_group_desc *
ext2_inode_group(uint32_t inode, struct ext2_mount_info *info) {
    return info->bg + ext2_inode_block_group(inode, info->sp);
}

static inline uint32_t
ext2_inode_block_count(struct ext2_inode *inode,
                       struct ext2_superblock *sp) {
    return inode->in_blocks / (2 << sp->s_log_block_size);
}

static inline uint32_t
ext2_inode_blocks(uint32_t block_count, struct ext2_superblock *sp) {
    return block_count * (2 << sp->s_log_block_size);
}

// directory entry

#define EXT2_DIR_BASE_SIZE 8

/**
 * Functions
 */

// global

int ext2_mount(void *fs, struct ext2_mount_info *info);
uint32_t ext2_lookup(ino_t ino, const char *fname, struct mount_info*);
int32_t  ext2_stat(ino_t ino, struct stat *st, struct ext2_mount_info *info);
int32_t  ext2_read(ino_t ino, void *buf, off_t offset, size_t len,
                   struct ext2_mount_info *info);
int32_t  ext2_write(ino_t ino, const void *buf, off_t offset, size_t len,
                    struct ext2_mount_info *info);

void     ext2_opench(ino_t, chann_adt_t*, struct mount_info*);
void     ext2_open(ino_t ino, vfile_t*, struct mount_info*);
void     ext2_close(ino_t ino, struct mount_info*);

// blocks

uint32_t ext2_block_alloc(struct ext2_mount_info *info);
void     ext2_block_free(uint32_t block, struct ext2_mount_info *info);

// inodes

struct ext2_inode *
ext2_get_inode(uint32_t inode, struct ext2_mount_info *info);

uint32_t ext2_lookup_free_inode(struct ext2_mount_info *info);
uint32_t ext2_alloc_inode(uint16_t type, uint16_t uid,
                          struct ext2_mount_info *info);

uint32_t ext2_inode_free(uint32_t inode, struct ext2_mount_info *info);

uint32_t ext2_alloc_inode_block(struct ext2_inode *inode,
                                uint32_t blknb, uint32_t block,
                                struct ext2_mount_info *info);

// create new file
uint32_t ext2_touch(uint32_t parent, const char *fname, uint16_t type,
                    struct ext2_mount_info *info);

// set file size to 0
uint32_t ext2_truncate(uint32_t ino, struct ext2_mount_info *info);

// functions to find blocks
uint32_t *ext2_get_inode_block_ptr(uint32_t block,
                                   struct ext2_inode *inode,
                                   struct ext2_mount_info *info);

uint32_t ext2_get_inode_block_nb(uint32_t block,
                                 struct ext2_inode *inode,
                                 struct ext2_mount_info *info);

block_t ext2_get_inode_block(uint32_t block,
                             struct ext2_inode *inode,
                             struct ext2_mount_info *info);

void ext2_set_inode_block(uint32_t nb, uint32_t block,
                          struct ext2_inode *inode,
                          struct ext2_mount_info *info);

// dir

struct dirent *
ext2_iter_dir(struct ext2_inode *inode,
              int (*iterator)(struct dirent*),
              struct ext2_mount_info *info);

uint32_t ext2_lookup_dir(struct ext2_inode *inode, const char *fname,
                         struct ext2_mount_info *info);

struct dirent *ext2_readdir(struct dirent *dir);

uint32_t ext2_add_dirent(uint32_t parent, uint32_t file,
                         const char *fname, struct ext2_mount_info*);

uint32_t ext2_mkdir(uint32_t parent, const char *dirname, uint16_t type,
                    struct ext2_mount_info *info);

int      ext2_getdents(ino_t, struct dirent* dst, size_t sz,
                        chann_adt_t*, struct mount_info*);

// links

ino_t ext2_link(ino_t target, ino_t parent, const char *fname,
                struct ext2_mount_info*);

ino_t ext2_symlink(ino_t parent, const char *name, const char *target,
                   struct ext2_mount_info*);

int ext2_readlink(ino_t ino, char *buf, size_t, struct ext2_mount_info*);

#endif
