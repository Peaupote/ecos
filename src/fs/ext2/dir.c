#include <stddef.h>
#include <fs/ext2.h>
#include <util/string.h>

#include <stdio.h>
#include <string.h>

struct ext2_dir_entry *
ext2_iter_dir(struct ext2_inode *inode,
              int (*iterator)(struct ext2_dir_entry*),
              struct ext2_mount_info *info) {
    size_t nbblk = 0, len = 0;
    struct ext2_dir_entry *dir;

    for (dir = ext2_get_inode_block(0, inode, info);
         len < inode->in_size;
         dir = ext2_readdir(dir)) {
        if (!dir->d_ino) dir = ext2_get_inode_block(++nbblk, inode, info);

        len += dir->d_rec_len;
        if (iterator(dir) < 0) return dir;
    }

    return 0;
}


static char *lookup_name;
static int cmp_dir_name(struct ext2_dir_entry* dir) {
    if (!ustrncmp(dir->d_name, lookup_name, 255)) return -1;
    return 0;
}

struct ext2_inode *
ext2_lookup_dir(struct ext2_inode *inode, char *fname,
                struct ext2_mount_info *info) {
    lookup_name = fname;
    struct ext2_dir_entry *entry = ext2_iter_dir(inode, cmp_dir_name, info);

    // TODO : binary search

    if (!entry) return 0;

    printf("%d\n", entry->d_ino);
    return ext2_find_inode(entry->d_ino, info);
}

struct ext2_dir_entry *
ext2_readdir(struct ext2_dir_entry *dir) {
    // assert (dir->rec_len < sizeof(struct ext2_dir_entry));
    return (struct ext2_dir_entry*)((char*)dir + dir->d_rec_len);
}

struct ext2_dir_entry *
ext2_mkdir(struct ext2_inode *parent, char *dirname,
           struct ext2_mount_info *info) {
    if (!(parent->in_type&EXT2_TYPE_DIR)) return 0;

    size_t strlen = ustrlen(dirname) + 1;
    if (strlen > 256) return 0;

    size_t nbblk = 0, len = 0, len_in_block = 0;
    struct ext2_dir_entry *dir;

    for (dir = ext2_get_inode_block(0, parent, info); len < parent->in_size;
         dir = ext2_readdir(dir)) {
        if (!dir->d_ino) {
            len_in_block = 0;
            dir = ext2_get_inode_block(++nbblk, parent, info);
        }

        len += dir->d_rec_len;
        len_in_block += dir->d_rec_len;
    }

    // alignement
    size_t rec_len;
    for (rec_len = EXT2_DIR_BASE_SIZE + strlen; len&0x2; len++);

    if (len_in_block + rec_len >= info->block_size) {
        if (len >= parent->in_size) {
            printf("alloc\n");
            ext2_block_alloc(parent, info);
        }
        dir = ext2_get_inode_block(++nbblk, parent, info);
    } else dir = ext2_readdir(dir);

    dir->d_name_len = strlen;
    dir->d_rec_len = rec_len;
    parent->in_size += rec_len;
    memcpy(dir->d_name, dirname, strlen);

    printf("%d (%d) %*s\n", dir->d_ino, dir->d_rec_len,
           dir->d_name_len, dir->d_name);

    return dir;
}
