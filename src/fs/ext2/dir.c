#include <stddef.h>
#include <fs/ext2.h>
#include <libc/string.h>

#if defined(__is_test)
#include <stdio.h>
#include <string.h>
#endif

#if defined(__is_kernel)
#include <kernel/kutil.h>
#endif

struct ext2_dir_entry *
ext2_iter_dir(struct ext2_inode *inode,
              int (*iterator)(struct ext2_dir_entry*),
              struct ext2_mount_info *info) {
    size_t nbblk = 0, len = 0, len_in_block = 0;
    struct ext2_dir_entry *dir;

    dir = ext2_get_inode_block(0, inode, info);
    goto start;

    do {
        if (len_in_block == info->block_size) {
            len_in_block = 0;
            dir = ext2_get_inode_block(++nbblk, inode, info);
        } else dir = ext2_readdir(dir);
    start:

        if (iterator(dir) < 0) break;

        len += dir->d_rec_len;
        len_in_block += dir->d_rec_len;
    } while (len < inode->in_size);

    if (len == inode->in_size) return 0;

    return dir;
}

static const char *lookup_name;
static int cmp_dir_name(struct ext2_dir_entry* dir) {
    if (!strncmp(dir->d_name, lookup_name, 256)) return -1;
    return 0;
}

uint32_t ext2_lookup_dir(struct ext2_inode *inode, const char *fname,
                         struct ext2_mount_info *info) {
    lookup_name = fname;
    struct ext2_dir_entry *entry = ext2_iter_dir(inode, cmp_dir_name, info);
    // TODO : binary search
    return entry ? entry->d_ino : 0;
}

struct ext2_dir_entry *ext2_readdir(struct ext2_dir_entry *dir) {
    return (struct ext2_dir_entry*)((char*)dir + dir->d_rec_len);
}

static struct ext2_dir_entry *
ext2_reach_end(struct ext2_inode *parent, struct ext2_mount_info *info) {
    struct ext2_dir_entry *dir;
    uint32_t b;
    uint32_t blk = parent->in_size / info->block_size;
    uint32_t off = parent->in_size % info->block_size;

    if (blk && !off) {
        if (!(b = ext2_block_alloc(info))) return 0;

        if (!ext2_alloc_inode_block(parent, blk, b, info)) {
            ext2_block_free(b, info);
            return 0;
        }

        dir = ext2_get_block(b, info);
        off = 0;
    } else {
        for (dir = ext2_get_inode_block(blk, parent, info);
             dir->d_ino; dir = ext2_readdir(dir));
    }

    return dir;
}

uint32_t ext2_add_dirent(struct ext2_inode *parent, uint32_t file,
                         const char *fname, struct ext2_mount_info *info) {
    struct ext2_dir_entry *dir;
    uint32_t name_len, rec_len;

    name_len = strlen(fname) + 1;
    for (rec_len = EXT2_DIR_BASE_SIZE + name_len; rec_len&3; rec_len++);

    dir = ext2_reach_end(parent, info);

    struct ext2_inode *inode = ext2_get_inode(file, info);
    dir->d_ino       = file;
    dir->d_file_type = inode->in_type;
    dir->d_name_len  = name_len;
    dir->d_rec_len   = rec_len;
    memcpy(dir->d_name, fname, dir->d_name_len);
    dir->d_name[dir->d_name_len] = 0;

    if ((parent->in_size % info->block_size) + rec_len < info->block_size) {
        dir = ext2_readdir(dir);
        dir->d_ino = 0;
    }

    parent->in_size += rec_len;

    return file;
}

uint32_t ext2_mkdir(uint32_t parent, const char *dirname, uint16_t type,
                    struct ext2_mount_info *info) {
    struct ext2_inode *p = ext2_get_inode(parent, info);
    struct ext2_dir_entry *dir;
    uint32_t name_len, rec_len, ino, block;

    if (!(type&EXT2_TYPE_DIR) || !(p->in_type&EXT2_TYPE_DIR)) return 0;

    if (!(ino = ext2_lookup_free_inode(info))) return 0;
    if (!(block = ext2_block_alloc(info))) {
        ext2_inode_free(ino, info);
        return 0;
    }

    struct ext2_inode *inode = ext2_get_inode(ino, info);
    inode->in_block[0] = block;

    dir = ext2_get_block(block, info);
    dir->d_ino = ino;
    dir->d_rec_len = EXT2_DIR_BASE_SIZE + 4;
    dir->d_name_len = 1;
    dir->d_file_type = type;
    memcpy(dir->d_name, ".", 2);

    inode->in_size += dir->d_rec_len;

    dir = ext2_readdir(dir);
    dir->d_ino = parent;
    dir->d_rec_len = EXT2_DIR_BASE_SIZE + 4;
    dir->d_name_len = 2;
    dir->d_file_type = p->in_type;
    memcpy(dir->d_name, "..", 3);

    dir = ext2_readdir(dir);
    dir->d_ino = 0;

    // write in parent
    dir = ext2_reach_end(p, info);
    name_len = strlen(dirname);
    for (rec_len = EXT2_DIR_BASE_SIZE + name_len; rec_len&2; rec_len++);

    dir->d_ino      = ino;
    dir->d_name_len = name_len;
    dir->d_rec_len  = rec_len;
    memcpy(dir->d_name, dirname, name_len);
    p->in_size += rec_len;

    uint32_t g = ext2_inode_block_group(ino, info->sp);
    struct ext2_group_desc *group = info->bg + g;
    group->g_dir_count++;

    return dir->d_ino;
}

struct ext2_dir_entry *
ext2_opendir(uint32_t ino, struct ext2_mount_info *info) {
    struct ext2_inode *inode = ext2_get_inode(ino, info);
    struct ext2_dir_entry *dir = 0;
    if (inode->in_type&EXT2_TYPE_DIR) {
        dir = (struct ext2_dir_entry*)ext2_get_inode_block(0, inode, info);
    }

    return dir;
}
