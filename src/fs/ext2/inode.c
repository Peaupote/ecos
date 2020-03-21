#include <fs/ext2.h>

struct ext2_inode *
ext2_find_inode(uint32_t inode, struct ext2_mount_info *info) {
    struct ext2_group_desc *bd = ext2_find_inode_group(inode, info);
    uint32_t index = ext2_inode_index(inode, info->sp);
    uint32_t offset = (bd->g_inode_table - 1) * info->block_size;
    offset += index * info->sp->s_inode_size;

    return (struct ext2_inode*)((char*)info->sp + offset);
}

struct block *
ext2_get_inode_block (uint32_t block,
                 struct ext2_inode *inode,
                 struct ext2_mount_info *info) {
    uint32_t inf = 0;
    uint32_t sup = EXT2_DIRECT_BLOCK;
    uint32_t *b; // indirection block

    if (block < sup) {
        return ext2_get_block(inode->in_block[block], info);
    }

    inf = sup;
    sup += info->block_size >> 2;
    if (block < sup) {
        b = (uint32_t*)ext2_get_block(inode->in_block[13], info);
        return ext2_get_block(b[block - inf], info);
    }

    return 0; // TODO : finish indirection
}
