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
    info->sp = fs + 1024;
    info->bg = (struct ext2_group_desc*)(info->sp + 1);
    info->group_count = ext2_group_count(info->sp);
    info->block_size = ext2_block_size(info->sp);
    info->group_size = info->block_size * info->sp->s_blocks_per_group;

    if (info->group_count != ext2_group_count_inodes(info->sp)) {
        return -1;
    }

    return 0;
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
