#include <fs/ext2.h>

static int is_power_of(uint32_t x, uint32_t n) {
    while (x > 1 && x % n == 0) x /= n;
    return x == 1;
}

int is_superblock_sparse(uint32_t x) {
    return x == 1 || is_power_of(x, 3) ||
        is_power_of(x, 5) || is_power_of(x, 7);
}

int ext2_mount(void *fs, struct ext2_mount_info *info) {
    info->sp = (struct ext2_superblock*)((char*)fs + 1024);
    info->bg = (struct ext2_group_desc*)(info->sp + 1);
    info->group_count = ext2_group_count(info->sp);
    info->block_size = ext2_block_size(info->sp);
    info->group_size = info->block_size * info->sp->s_blocks_per_group;
    info->root_ino = EXT2_ROOT_INO;

    if (info->group_count != ext2_group_count_inodes(info->sp)) {
        return 0;
    }

    return 1;
}

uint32_t ext2_block_alloc(struct ext2_mount_info *info) {
    uint32_t g = 0;
    struct ext2_group_desc *group;

    // TODO : adapt to make block contiguous

    for (group = info->bg + g;
         g < info->group_count && group->g_free_blocks_count == 0;
         group++, g++);

    uint8_t *bitmap = ext2_get_block(group->g_block_bitmap, info);
    uint32_t block, rem = 0;

    for (block = 0; block < info->sp->s_blocks_per_group; block++) {
        rem = block & 0b111;
        if (block != 0 && rem == 0) bitmap++;
        if (0 == (*bitmap & (1 << rem))) break;
    }

    // TODO : try an other group
    if (block == info->sp->s_blocks_per_group) return 0;

    *bitmap |= 1 << rem;
    group->g_free_blocks_count--;

    return block + 1;
}

uint32_t ext2_lookup(const char *fname, ino_t ino,
                     struct ext2_mount_info *info) {
    struct ext2_inode *inode = ext2_get_inode(ino, info);
    if (!(inode->in_type&EXT2_TYPE_DIR)) return 0;

    return ext2_lookup_dir(inode, fname, info);
}

int ext2_stat(ino_t ino, struct stat *st, struct ext2_mount_info *info) {
    struct ext2_inode *inode = ext2_get_inode(ino, info);
    st->st_ino     = ino;
    st->st_mode    = inode->in_type;
    st->st_nlink   = inode->in_hard;
    st->st_uid     = inode->in_uid;
    st->st_gid     = inode->in_gid;
    st->st_size    = inode->in_size;
    st->st_blksize = info->block_size;
    st->st_blocks  = inode->in_blocks;
    st->st_ctime   = inode->in_ctime;
    st->st_mtime   = inode->in_mtime;
    return 1;
}

int ext2_read(ino_t ino, void *buf, off_t pos, size_t len,
              struct ext2_mount_info *info) {
    struct ext2_inode *inode = ext2_get_inode(ino, info);
    uint32_t block_nb = pos / info->block_size;
    char *block = ext2_get_inode_block(block_nb, inode, info);
    char *dst = (char*)buf;

    size_t i;
    for (i = 0; i < len && pos < inode->in_size; i++) {
        *dst++ = block[pos++ % info->block_size];
        if (pos % info->block_size == 0) {
            block = ext2_get_inode_block(++block_nb, inode, info);
        }
    }

    return i;
}

int ext2_write(ino_t ino __attribute__((unused)),
               void *buf __attribute__((unused)),
               off_t pos __attribute__((unused)),
               size_t len __attribute__((unused)),
               struct ext2_mount_info *info __attribute__((unused))) {
    return 0;
}
