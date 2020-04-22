#include <stddef.h>

#include <fs/ext2.h>
#include <string.h>

#if defined(__is_kernel)
#include <kernel/kutil.h>
#endif

#include <util/misc.h>

#include <util/test.h>

struct dirent *
ext2_iter_dir(struct ext2_inode *inode,
              int (*iterator)(struct dirent*),
              struct ext2_mount_info *info) {
    size_t nbblk = 0, len = 0, len_in_block = 0;
    struct dirent *dir;

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
static int cmp_dir_name(struct dirent* dir) {
    if (!strncmp(lookup_name, dir->d_name, dir->d_name_len) &&
        !lookup_name[dir->d_name_len])
        return -1;
    return 0;
}

uint32_t ext2_lookup_dir(struct ext2_inode *inode, const char *fname,
                         struct ext2_mount_info *info) {
    lookup_name = fname;
    struct dirent *entry = ext2_iter_dir(inode, cmp_dir_name, info);
    // TODO : binary search
    return entry ? entry->d_ino : 0;
}

struct dirent *ext2_readdir(struct dirent *dir) {
    return (struct dirent*)((char*)dir + dir->d_rec_len);
}

uint32_t ext2_add_dirent(uint32_t parent, uint32_t file,
                         const char *fname, struct ext2_mount_info *info) {
    struct dirent dir;
    uint32_t name_len, rec_len;
    struct ext2_inode *p = ext2_get_inode(parent, info);

    name_len = strlen(fname);
    for (rec_len = EXT2_DIR_BASE_SIZE + name_len; rec_len&3; ++rec_len);

    uint32_t pos = 0;
    while (pos < p->in_size) {
        ext2_read(parent, &dir, pos, DIRENT_OFF, info);
        pos += dir.d_rec_len;
    }

    struct ext2_inode *inode = ext2_get_inode(file, info);
    dir.d_ino       = file;
    dir.d_file_type = inode->in_type;
    dir.d_name_len  = name_len;
    dir.d_rec_len   = rec_len;

    uint32_t sz = p->in_size;
    ext2_write(parent, &dir, pos, DIRENT_OFF, info);
    ext2_write(parent, fname, pos + DIRENT_OFF, name_len, info);
    p->in_size = sz + rec_len;

    ++inode->in_hard; // one more link to file
    return file;
}

uint32_t ext2_mkdir(uint32_t parent, const char *dirname, uint16_t type,
                    struct ext2_mount_info *info) {
    uint32_t ino;

    if ((ino = ext2_lookup(parent, dirname, (struct mount_info*)info)) &&
        (ext2_get_inode(ino, info)->in_type&TYPE_DIR)) return 0;

    if (!(type&TYPE_DIR)) return 0;

    struct ext2_inode *p = ext2_get_inode(parent, info);
    if (!(p->in_type&TYPE_DIR)) return 0;
    if (!(ino = ext2_lookup_free_inode(info))) return 0;

    ino = ext2_alloc_inode(type, 0, info);
    if (!ext2_add_dirent(parent, ino, dirname, info) ||
        !ext2_add_dirent(ino, ino, ".", info) ||
        !ext2_add_dirent(ino, parent, "..", info)) return 0;

    return ino;
}

struct ext2_cdt_dir { //sz <= CADT_SIZE
    uint32_t size;
    uint32_t pos;
};

void ext2_opench(ino_t ino, chann_adt_t* p_cdt,
                    struct mount_info* p_info) {
    struct ext2_mount_info* info = (struct ext2_mount_info*) p_info;
    struct ext2_inode     *inode = ext2_get_inode(ino, info);

    if (inode->in_type & EXT2_TYPE_DIR) {
        struct ext2_cdt_dir* cdt = (struct ext2_cdt_dir*) p_cdt;
        cdt->pos  = 0;
        cdt->size = inode->in_size;
    }
}

int ext2_getdents(ino_t ino, struct dirent* dst, size_t sz,
                  chann_adt_t* p_cdt, struct mount_info* none) {
    struct ext2_cdt_dir *cdt     = (struct ext2_cdt_dir*)p_cdt;
    struct ext2_mount_info *info = (struct ext2_mount_info*)none;

    int rc = 0;
    while (cdt->pos < cdt->size && sz >= offsetof(struct dirent, d_name)) {
        struct dirent src;
        ext2_read(ino, &src, cdt->pos, DIRENT_OFF, info);

        if (sz < src.d_rec_len) {
            if (rc) return rc;
            memcpy(dst, &src, offsetof(struct dirent, d_name));
            return offsetof(struct dirent, d_name);
        }

        ext2_read(ino, dst, cdt->pos, src.d_rec_len, info);
        cdt->pos += src.d_rec_len;
        rc      += src.d_rec_len;
        sz      -= src.d_rec_len;
        dst = (struct dirent*)(src.d_rec_len + (char*)dst);
    }
    return rc;
}
