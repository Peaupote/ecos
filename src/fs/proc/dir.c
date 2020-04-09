#include <headers/unistd.h>

#include <kernel/file.h>
#include <kernel/proc.h>

#include <fs/proc.h>
#include <fs/pipe.h>

#include <libc/string.h>

#include "special.h"

uint32_t proc_add_dirent(struct proc_inode *p, int32_t ino, const char *fname) {
    struct ext2_dir_entry *dir = (struct ext2_dir_entry*)p->in_block[0];
    while (dir->d_ino)
		dir = (struct ext2_dir_entry*)(dir->d_rec_len + (char*)dir);
    proc_fill_dirent(dir, ino, fname);
    p->st.st_size += dir->d_rec_len;

    proc_inodes[ino].st.st_nlink++;

	dir = (struct ext2_dir_entry*)(dir->d_rec_len + (char*)dir);
    proc_fill_dirent(dir, 0, "");
    return ino;
}

uint32_t proc_init_dir(uint32_t ino, uint32_t parent, uint16_t type) {
    struct proc_inode *inode = proc_inodes + ino;
    uint32_t b = proc_free_block();
    if (b == PROC_NBLOCKS) {
        klogf(Log_error, "procfs", "no free blocks");
        return 0;
    }

    inode->in_block[0] = (uint64_t)(proc_blocks + b);
    inode->st.st_size  = 0;

    struct ext2_dir_entry *dir = (struct ext2_dir_entry*)inode->in_block[0];
    proc_fill_dirent(dir, ino, ".");
    inode->st.st_size += dir->d_rec_len;

	dir = (struct ext2_dir_entry*)(dir->d_rec_len + (char*)dir);
    proc_fill_dirent(dir, parent, "..");
    proc_inodes[parent].st.st_nlink++;
    inode->st.st_size += dir->d_rec_len;

	dir = (struct ext2_dir_entry*)(dir->d_rec_len + (char*)dir);
    proc_fill_dirent(dir, 0, "");

    inode->st.st_mode  = type;
    inode->st.st_uid   = 0;
    inode->st.st_ctime = 0; // TODO now
    inode->st.st_mtime = 0;
    inode->st.st_gid   = 0;
    inode->st.st_nlink = 1;

    // set block b to used
    proc_block_bitmap[b >> 3] |= 1 << (b&7);
    return ino;
}
uint32_t proc_lookup(const char *fname, ino_t parent,
                     struct mount_info *info) {
    struct proc_inode *inode = proc_inodes + parent;
    if (!(inode->st.st_mode&TYPE_DIR)) return 0;

    size_t nbblk = 0, len = 0, len_in_block = 0;
    struct ext2_dir_entry *dir = (struct ext2_dir_entry*)inode->in_block[0];
    goto start;

    do {
        if (len_in_block == info->block_size) {
            len_in_block = 0;
            if (++nbblk >= PROC_NBLOCK) return 0;
            dir = (struct ext2_dir_entry*)inode->in_block[++nbblk];
        } else
			dir = (struct ext2_dir_entry*)(dir->d_rec_len + (char*)dir);

    start:
        if (!dir->d_ino) break;
        if (!strncmp(fname, dir->d_name, dir->d_name_len) &&
            !fname[dir->d_name_len]) {
            break;
        }

        len += dir->d_rec_len;
        len_in_block += dir->d_rec_len;
    } while(len < inode->st.st_size);

    if (len == inode->st.st_size) return 0;

    return dir->d_ino;
}

static struct dirent_it* proc_dirent_from_ext2(struct dirent_it *d,
		struct ext2_dir_entry* ed) {
	struct dirent* c = &d->cnt;
	c->d_ino       = ed->d_ino;
	c->d_rec_len   = ed->d_rec_len;
	c->d_name_len  = ed->d_name_len;
	c->d_file_type = ed->d_file_type;
	c->d_name      = ed->d_name;
	struct ext2_dir_entry** nx = (struct ext2_dir_entry**) d->dt;
	*nx = (struct ext2_dir_entry*)(ed->d_rec_len + (char*)ed);
    return d;
}

struct dirent_it *proc_readdir(struct dirent_it *dir,
		char* nbuf __attribute__((unused))) {
	struct ext2_dir_entry** nx = (struct ext2_dir_entry**) dir->dt;
    return proc_dirent_from_ext2(dir, *nx);
}

uint32_t proc_mkdir(ino_t parent, const char *fname, uint16_t type,
                    struct mount_info *info) {
    uint32_t ino;

    // test if file already exists and is a directory
    if ((ino = proc_lookup(fname, parent, info)) &&
        (proc_inodes[ino].st.st_mode&TYPE_DIR)) {
        klogf(Log_error, "procfs", "mkdir %s already exists", fname);
        return 0;
    }

    // create only dir with mkdir
    if (!(type&TYPE_DIR)) {
        klogf(Log_error, "procfs", "mkdir create only directory");
        return 0;
    }

    struct proc_inode *p = proc_inodes + parent;
    if (!(p->st.st_mode&TYPE_DIR)) {
        klogf(Log_error, "procfs", "parent (ino %d) is not a directory");
        return 0;
    }

    if (!(ino = proc_free_inode())) {
        klogf(Log_error, "procfs", "no free inodes");
        return 0;
    }

    if (!proc_init_dir(ino, parent, type)) {
        klogf(Log_error, "procfs", "failed to init dir %d in parent %d",
              ino, parent);
        return 0;
    }

    if (!proc_add_dirent(p, ino, fname)) {
        klogf(Log_error, "procfs", "failed to add dirent %d in parent %d",
              ino, parent);
        return 0;
    }

    klogf(Log_verb, "procfs", "mkdir %s inode %d", fname, ino);

    return ino;
}

struct dirent_it *
proc_opendir(ino_t ino, struct dirent_it* it,
		char* nbuf __attribute__((unused)), 
		struct mount_info *info __attribute__((unused))) {

    struct proc_inode *inode = proc_inodes + ino;
    return proc_dirent_from_ext2(it, (struct ext2_dir_entry*)inode->in_block[0]);
}

ino_t proc_destroy_dirent(ino_t p, ino_t ino, struct mount_info *info) {
    struct proc_inode *parent = proc_inodes + p;
    struct ext2_dir_entry *ndir, *dir;
    size_t size, rec_len;

    dir = (struct ext2_dir_entry*)parent->in_block[0];

    for (size = 0; size < parent->st.st_size;
         size += dir->d_rec_len,
		 dir = (struct ext2_dir_entry*)(dir->d_rec_len + (char*)dir)) {
        if (dir->d_ino == ino) break;
    }

    if (size == parent->st.st_size) {
        kAssert(false);
        return 0;}

    rec_len = dir->d_rec_len;

    if (size + rec_len == parent->st.st_size) goto ret;

	ndir = (struct ext2_dir_entry*)(dir->d_rec_len + (char*)dir);
    memmove(dir, ndir, parent->st.st_size - size - rec_len);

ret:
    // TODO if not in this device
    proc_rm(ino, info);
    parent->st.st_size -= rec_len;
    return ino;
}
