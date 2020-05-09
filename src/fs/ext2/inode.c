#include <fs/ext2.h>

#include <libc/string.h>
#include <util/test.h>

struct ext2_inode *
ext2_get_inode(uint32_t inode, struct ext2_mount_info *info) {
    struct ext2_group_desc *bd = ext2_inode_group(inode, info);
    uint32_t index = ext2_inode_index(inode, info->sp);
    uint32_t offset = (bd->g_inode_table - 1) * info->block_size;
    offset += index * info->sp->s_inode_size;

    return (struct ext2_inode*)((char*)info->sp + offset);
}

uint32_t
ext2_lookup_free_inode(struct ext2_mount_info *info) {
    uint32_t g = 0;
    struct ext2_group_desc *group;

    for (group = info->bg + g; g < info->group_count; group++, g++) {
        if (group->g_free_inodes_count == 0) continue;

        uint8_t *bitmap = ext2_get_block(group->g_inode_bitmap, info);
        uint32_t inode, rem = 0;

        for (inode = 0; inode < info->sp->s_inodes_per_group; inode++) {
            rem = inode&7;
            if (inode != 0 && rem == 0) bitmap++;
            if (0 == (*bitmap & (1 << rem))) break;
        }

        *bitmap |= 1 << rem;
        group->g_free_inodes_count--;

        return g * info->sp->s_inodes_per_group + inode + 1;
    }

    return 0;
}

uint32_t
ext2_alloc_inode(uint16_t type, uint16_t uid,
                 struct ext2_mount_info *info) {
    uint32_t ino;
    if (!(ino = ext2_lookup_free_inode(info))) return 0;

    struct ext2_inode *inode = ext2_get_inode(ino, info);
    inode->in_type = type;
    inode->in_uid  = uid;
    inode->in_size = 0;

    inode->in_atime = 0;
    inode->in_ctime = 0;
    inode->in_mtime = 0;
    inode->in_dtime = 0;

    inode->in_gid    = 0;
    inode->in_hard   = 0;
    inode->in_blocks = 0;
    inode->in_flags  = 0;
    inode->in_os     = EXT2_OS_OTHER;

    return ino;
}

uint32_t *ext2_get_inode_block_ptr(uint32_t block,
                                   struct ext2_inode *inode,
                                   struct ext2_mount_info *info) {
    uint32_t *b; // indirection block
    uint32_t bk = info->block_size >> 2; // number of block number per block

    // no indirection
    if (block < EXT2_DIRECT_BLOCK) return inode->in_block + block;

    // one indirection
    block -= EXT2_DIRECT_BLOCK;
    if (block < bk) {
        b = ext2_get_block(inode->in_block[12], info);
        return b + block;
    }

    // two indirections
    // two level of indirection is far from enough for our usage
    block -= bk;
    b = ext2_get_block(inode->in_block[13], info);
    b += block / bk;
    b = ext2_get_block(*b, info);
    return b + (block % bk);
}

uint32_t ext2_get_inode_block_nb(uint32_t block,
                                 struct ext2_inode *inode,
                                 struct ext2_mount_info *info) {
    uint32_t *b = ext2_get_inode_block_ptr(block, inode, info);
    return *b;
}

block_t ext2_get_inode_block(uint32_t block,
                             struct ext2_inode *inode,
                             struct ext2_mount_info *info) {
    uint32_t b = ext2_get_inode_block_nb(block, inode, info);
    return ext2_get_block(b, info);
}

void ext2_set_inode_block(uint32_t nb, uint32_t block,
                          struct ext2_inode *inode,
                          struct ext2_mount_info *info) {
    uint32_t *b = ext2_get_inode_block_ptr(nb, inode, info);
    *b = block;
}

uint32_t ext2_touch(uint32_t parent, const char *fname, uint16_t type,
                    struct ext2_mount_info *info) {

    uint32_t ino;

    if ((ino = ext2_lookup(parent, fname, (struct mount_info*)info)) &&
        !(ext2_get_inode(ino, info)->in_type&TYPE_DIR)) return 0;

    if (type&TYPE_DIR) return 0;

    struct ext2_inode *p = ext2_get_inode(parent, info);
    if (!(p->in_type&TYPE_DIR)) return 0;

    ino = ext2_alloc_inode(type, 0, info);
    if (!ext2_add_dirent(parent, ino, fname, info)) return 0;

    return ino;
}

uint32_t ext2_inode_free(uint32_t ino, struct ext2_mount_info *info) {
    uint32_t group = ext2_inode_block_group(ino, info->sp);
    struct ext2_group_desc *bg = info->bg + group;
    uint8_t *bitmap = ext2_get_block(bg->g_inode_bitmap, info);

    ino -= group * info->sp->s_inodes_per_group;
    bitmap[ino >> 3] &= ~(1 << (ino & 7));

    return 0;
}

uint32_t ext2_truncate(uint32_t ino, struct ext2_mount_info *info) {
    struct ext2_inode *inode = ext2_get_inode(ino, info);
    if (!inode->in_size) return ino;

    uint32_t nbblk = inode->in_size / info->block_size;
    if (inode->in_size % info->block_size) ++nbblk;

    for (uint32_t b = 0; b < nbblk; ++b) {
        uint32_t block = ext2_get_inode_block_nb(b, inode, info);

        uint32_t group = block / info->sp->s_blocks_per_group;
        struct ext2_group_desc *bg = info->bg + group;
        uint8_t *bitmap = ext2_get_block(bg->g_block_bitmap, info);
        bitmap[block >> 3] &= ~(1 << (block&7));
    }

    inode->in_size = 0;
    inode->in_blocks = 0;
    return ino;
}
