#include <stddef.h>
#include <fs/ext2.h>
#include <libc/string.h>

#include <stdio.h>
#include <string.h>

struct ext2_dir_entry *
ext2_iter_dir(struct ext2_inode *inode,
              int (*iterator)(struct ext2_dir_entry*),
              size_t *read,
              struct ext2_mount_info *info) {
    size_t nbblk = 0, len = 0, len_in_block = 0;
    struct ext2_dir_entry *dir;

    dir = ext2_get_inode_block(0, inode, info);
    goto start;

    do {
        if (len_in_block == info->block_size) {
            len_in_block = 0;
            dir = ext2_get_inode_block(++nbblk, inode, info);
            printf("switch block\n");
        } else dir = ext2_readdir(dir);
    start:

        if (iterator(dir) < 0) break;

        len += dir->d_rec_len;
        len_in_block += dir->d_rec_len;
    } while (len < inode->in_size);

    if (read) *read = len;
    return dir;
}


static char *lookup_name;
static int cmp_dir_name(struct ext2_dir_entry* dir) {
    if (!strncmp(dir->d_name, lookup_name, 255)) return -1;
    return 0;
}

uint32_t ext2_lookup_dir(struct ext2_inode *inode, char *fname,
                         struct ext2_mount_info *info) {
    lookup_name = fname;
    struct ext2_dir_entry *entry = ext2_iter_dir(inode, cmp_dir_name,
                                                 0, info);
    // TODO : binary search
    return entry ? entry->d_ino : 0;
}

struct ext2_dir_entry *ext2_readdir(struct ext2_dir_entry *dir) {
    return (struct ext2_dir_entry*)((char*)dir + dir->d_rec_len);
}


static int void_iterator(struct ext2_dir_entry * d __attribute__((unused))) {
    return 0;
}

struct ext2_dir_entry * ext2_mkdir(uint32_t inode, char *dirname,
                                   struct ext2_mount_info *info) {
    struct ext2_inode *parent = ext2_get_inode(inode, info);
    struct ext2_dir_entry *dir;
    size_t str_len, rec_len, len, block;

    if (!(parent->in_type&EXT2_TYPE_DIR)) return 0;

    str_len = strlen(dirname) + 1;
    if (str_len > 256) return 0;


    ext2_iter_dir(parent, void_iterator, &len, info);
    block = len / info->block_size;

    // alignement
    for (rec_len = EXT2_DIR_BASE_SIZE + str_len; len&2; len++);

    // move to right position
    // and alloc a block if necessary
    dir = ext2_get_inode_block(block, parent, info);

    dir->d_ino = ext2_alloc_inode(EXT2_TYPE_DIR|0640, 0, info);
    dir->d_name_len = str_len;
    dir->d_rec_len = rec_len;
    parent->in_size += rec_len;
    memcpy(dir->d_name, dirname, str_len);

    // TODO : write . and .. in directory

    uint32_t g = ext2_inode_block_group(inode, info->sp);
    struct ext2_group_desc *group = info->bg + g;
    group->g_dir_count++;

    printf("%d (%d) %*s\n", dir->d_ino, dir->d_rec_len,
           dir->d_name_len, dir->d_name);

    return dir;
}
