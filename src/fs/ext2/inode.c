#include <fs/ext2.h>

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

    // TODO : adapt to make block contiguous

    for (group = info->bg + g;
         g < info->group_count && group->g_free_inodes_count == 0;
         group++, g++);

    uint8_t *bitmap = ext2_get_block(group->g_inode_bitmap, info);
    uint32_t inode, rem = 0;

    for (inode = 0; inode < info->sp->s_inodes_per_group; inode++) {
        rem = inode & 0b111;
        if (inode != 0 && rem == 0) bitmap++;
        if (0 == (*bitmap & (1 << rem))) break;
    }

    // TODO : try an other group
    if (inode == info->sp->s_inodes_per_group) return 0;

    *bitmap |= 1 << rem;
    group->g_free_inodes_count--;

    return inode + 1;
}

uint32_t
ext2_alloc_inode(uint16_t type, uint16_t uid,
                 struct ext2_mount_info *info) {
    uint32_t ino = ext2_lookup_free_inode(info);

    struct ext2_inode *inode = ext2_get_inode(ino, info);

    inode->in_type  = type;
    inode->in_uid   = uid;
    inode->in_ctime = 0; // TODO : change
    inode->in_size  = 0;

    return ino;
}

uint32_t *ext2_get_inode_block_ptr(uint32_t block,
                                   struct ext2_inode *inode,
                                   struct ext2_mount_info *info) {
    uint32_t inf = 0;
    uint32_t sup = EXT2_DIRECT_BLOCK;
    uint32_t *b; // indirection block

    test_printf("get inode block %d\n", block);

    if (block < sup) return inode->in_block + block;

    inf = sup;
    sup += info->block_size >> 2;
    if (block < sup) {
        b = ext2_get_block(inode->in_block[12], info);
        /* kprintf("block %d => %d => %d\n", block, block - inf, b[block-inf]); */
        return b + block - inf;
    }

    return 0; // TODO : finish indirection
}

uint32_t ext2_get_inode_block_nb(uint32_t block,
                                 struct ext2_inode *inode,
                                 struct ext2_mount_info *info) {
    uint32_t *b = ext2_get_inode_block_ptr(block, inode, info);

    if (block >= ext2_inode_block_count(inode, info->sp)) {
        inode->in_mtime = 2; // TODO : change
        inode->in_blocks =
            ext2_inode_blocks(inode->in_size/info->block_size+1,info->sp);
        *b = ext2_block_alloc(info);
    }

    return *b;
}

block_t ext2_get_inode_block(uint32_t block,
                             struct ext2_inode *inode,
                             struct ext2_mount_info *info) {
    return ext2_get_block(ext2_get_inode_block_nb(block, inode, info), info);
}
