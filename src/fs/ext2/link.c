#include <fs/ext2.h>
#include <libc/string.h>

uint32_t ext2_link(uint32_t target, uint32_t parent, const char *fname,
                   struct ext2_mount_info *info) {
    struct ext2_inode *p = ext2_get_inode(parent, info);
    if (!p || !(p->in_type&EXT2_TYPE_DIR)) return 0;

    return ext2_add_dirent(parent, target, fname, info);
}

// assume symlink targets are no longer than 60 bytes

uint32_t ext2_symlink(uint32_t parent, const char *name, const char *target,
                      struct ext2_mount_info *info) {
    uint32_t ino = ext2_touch(parent, name, EXT2_TYPE_SYM, info);
    if (ino) {
        struct ext2_inode *inode = ext2_get_inode(ino, info);
        inode->in_type = EXT2_TYPE_SYM | 0640;
        size_t len = strlen(target);
        len = len < 60 ? len : 60;
        strncpy((char*)inode->in_block, target, len);
        inode->in_size = len;
    }

    return ino;
}

int ext2_readlink(uint32_t ino, char *buf, size_t len,
                     struct ext2_mount_info *info) {
    struct ext2_inode *inode = ext2_get_inode(ino, info);
    if (!inode || (inode->in_type&0xf000) != EXT2_TYPE_SYM) return -1;

    len = inode->in_size < len ? inode->in_size : len;
    memcpy(buf, inode->in_block, len);
    return len;
}
