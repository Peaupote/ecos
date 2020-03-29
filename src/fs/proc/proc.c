#include <fs/proc.h>
#include <fs/pipe.h>
#include <libc/string.h>
#include <kernel/file.h>
#include <kernel/proc.h>

static uint32_t proc_free_block() {
    size_t b;
    for (b = 0; b < PROC_NBLOCKS; b++) {
        if (!(proc_block_bitmap[b >> 3] & (1 << (b&7)))) break;
    }
    return b;
}

static inline void
proc_fill_dirent(struct dirent *dir, uint32_t ino, const char *fname) {
    dir->d_ino = ino;
    dir->d_file_type = proc_inodes[ino].in_type;
    dir->d_name_len = strlen(fname);
    dir->d_rec_len = DIRENT_OFF + dir->d_name_len + 1;
    memcpy(dir->d_name, fname, dir->d_name_len);
    dir->d_name[dir->d_name_len] = 0;
}

static inline uint32_t
proc_add_dirent(struct proc_inode *p, int32_t ino, const char *fname) {
    struct dirent *dir = (struct dirent*)p->in_block[0];
    while (dir->d_ino) dir = proc_readdir(dir);
    proc_fill_dirent(dir, ino, fname);
    p->in_size += dir->d_rec_len;

    dir = proc_readdir(dir);
    proc_fill_dirent(dir, 0, "");
    return ino;
}

static uint32_t proc_init_dir(uint32_t ino, uint32_t parent, uint16_t type) {
    struct proc_inode *inode = proc_inodes + ino;
    uint32_t b = proc_free_block();
    if (b == PROC_NBLOCK) return 0;

    inode->in_block[0] = proc_blocks + b;

    struct dirent *dir = (struct dirent*)inode->in_block[0];
    proc_fill_dirent(dir, ino, ".");
    inode->in_size += dir->d_rec_len;

    dir = proc_readdir(dir);
    proc_fill_dirent(dir, parent, "..");
    inode->in_size += dir->d_rec_len;

    dir = proc_readdir(dir);
    proc_fill_dirent(dir, 0, "");

    inode->in_type  = type;
    inode->in_uid   = 0;
    inode->in_ctime = 0; // TODO now
    inode->in_mtime = 0;
    inode->in_gid   = 0;
    inode->in_hard  = 1;

    // set block b to used
    proc_block_bitmap[b >> 3] |= 1 << (b&7);
    return ino;
}

int proc_mount(void *partition __attribute__((unused)),
               struct mount_info *info) {
    for (size_t i = 0; i < NPROC; i++) {
        proc_inodes[i].in_size = 0;
        proc_inodes[i].in_hard = 0;
    }

    memset(&proc_block_bitmap, 0, PROC_BITMAP_SIZE);

    // irrelevant here
    info->sp = 0;

    info->block_size = PROC_BLOCK_SIZE;
    info->root_ino   = PROC_ROOT_INO;
    proc_init_dir(PROC_ROOT_INO, PROC_ROOT_INO, TYPE_DIR|0640);

    return 1;
}

uint32_t proc_lookup(const char *fname, ino_t parent,
                     struct mount_info *info) {
    struct proc_inode *inode = proc_inodes + parent;
    if (!(inode->in_type&TYPE_DIR)) return 0;

    size_t nbblk = 0, len = 0, len_in_block = 0;
    struct dirent *dir = (struct dirent*)inode->in_block[0];
    goto start;

    do {
        if (len_in_block == info->block_size) {
            len_in_block = 0;
            if (++nbblk >= PROC_NBLOCK) return 0;
            dir = (struct dirent*)inode->in_block[++nbblk];
        } else dir = proc_readdir(dir);

    start:
        if (!dir->d_ino) break;
        if (!strncmp(fname, dir->d_name, dir->d_name_len))
            break;

        len += dir->d_rec_len;
        len_in_block += dir->d_rec_len;
    } while(len < inode->in_size);

    if (len == inode->in_size) return 0;

    return dir->d_ino;
}

struct dirent *proc_readdir(struct dirent *dir) {
    return (struct dirent*)((char*)dir + dir->d_rec_len);
}

int proc_stat(ino_t ino, struct stat *st, struct mount_info *info) {
    struct proc_inode *inode = proc_inodes + ino;

    st->st_ino     = ino;
    st->st_mode    = inode->in_type;
    st->st_nlink   = inode->in_hard;
    st->st_uid     = inode->in_uid;
    st->st_gid     = inode->in_gid;
    st->st_size    = inode->in_size;
    st->st_blksize = info->block_size;
    st->st_blocks  = inode->in_size >> 9;
    st->st_ctime   = inode->in_ctime;
    st->st_mtime   = inode->in_mtime;

    return ino;
}

int proc_read(ino_t ino, void *buf, off_t pos __attribute__((unused)),
              size_t len, struct mount_info *info __attribute__((unused))) {
    struct proc_inode *inode = proc_inodes + ino;
    switch (inode->in_type&0xf000) {
    case TYPE_FIFO:
        return pipe_read((struct pipe_inode*)inode->in_block, buf, len);
        break;

    default:
        kAssert(false);
        return 0;
    }
}

int proc_write(ino_t ino, void *buf, off_t pos __attribute__((unused)),
              size_t len, struct mount_info *info __attribute__((unused))) {
    struct proc_inode *inode = proc_inodes + ino;
    switch (inode->in_type&0xf000) {
    case TYPE_FIFO:
        return pipe_write((struct pipe_inode*)inode->in_block, buf, len);
        break;

    default:
        kAssert(false);
        return 0;
    }
}

static inline uint32_t proc_free_inode() {
    uint32_t ino;
    for (ino = 2; ino < PROC_NINODES; ino++) {
        if (!proc_inodes[ino].in_hard) break;
    }

    return ino == PROC_NINODES ? 0 : ino;
}

uint32_t proc_touch(ino_t parent, const char *fname, uint16_t type,
                    struct mount_info *info) {
    uint32_t ino;

    // test if file already exists and is not a directory
    if ((ino = proc_lookup(fname, parent, info)) &&
        !(proc_inodes[ino].in_type&TYPE_DIR))
        return 0;

    // dont create directories with touch
    if (type&TYPE_DIR) return 0;

    struct proc_inode *p = proc_inodes + parent;
    if (!(p->in_type&TYPE_DIR)) return 0;

    if (!(ino = proc_free_inode())) return 0;
    struct proc_inode *inode = proc_inodes + ino;

    uint32_t b = proc_free_block();
    if (b == PROC_NBLOCK) return 0;
    inode->in_block[0] = proc_blocks + b;

    if (!proc_add_dirent(p, ino, fname)) return 0;

    inode->in_type  = type|TYPE_DIR;
    inode->in_uid   = 0;
    inode->in_size  = 0;
    inode->in_ctime = 0; // TODO now
    inode->in_mtime = 0;
    inode->in_gid   = 0;
    inode->in_hard  = 1;

    // set block b to used
    proc_block_bitmap[b >> 3] |= 1 << (b&7);

    klogf(Log_info, "procfs", "touch %s inode %d", fname, ino);

    return ino;
}

uint32_t proc_mkdir(ino_t parent, const char *fname, uint16_t type,
                    struct mount_info *info) {
    uint32_t ino;

    // test if file already exists and is a directory
    if ((ino = proc_lookup(fname, parent, info)) &&
        (proc_inodes[ino].in_type&TYPE_DIR)) {
        kprintf("return ino %d\n", ino);
        klogf(Log_error, "procfs", "mkdir %s already exists", fname);
        return 0;
    }

    // create only dir with mkdir
    if (!(type&TYPE_DIR)) {
        klogf(Log_error, "procfs", "mkdir create only directory");
        return 0;
    }

    struct proc_inode *p = proc_inodes + parent;
    if (!(p->in_type&TYPE_DIR)) {
        klogf(Log_error, "procfs", "parent (ino %d) is not a directory");
        return 0;
    }

    if (!(ino = proc_free_inode())) {
        klogf(Log_error, "procfs", "no free inodes");
        return 0;
    }

    if (!proc_add_dirent(p, ino, fname)) return 0;
    if (!proc_init_dir(ino, parent, type)) return 0;

    klogf(Log_info, "procfs", "mkdir %s inode %d", fname, ino);

    return ino;
}

struct dirent *
proc_opendir(ino_t ino, struct mount_info *info __attribute__((unused))) {
    struct proc_inode *inode = proc_inodes + ino;
    return (struct dirent*)inode->in_block[0];
}

uint32_t proc_create(pid_t pid) {
    struct device *dev = find_device("/proc");
    char buf[256];
    uint32_t ino, rc;

    sprintf(buf, "%d", pid);
    ino = proc_mkdir(PROC_ROOT_INO, buf,
                     TYPE_DIR|0400, &dev->dev_info);
    if (!ino) return 0;

    rc = proc_mkdir(ino, "fd", TYPE_DIR|0640, &dev->dev_info);
    if (!rc) return 0;

    proc_t *p = state.st_proc + pid;
    struct dirent *dir = proc_opendir(rc, &dev->dev_info);
    while (dir->d_ino) dir = proc_readdir(dir);

    for (int i = 0; i < NFD; i++) {
        if (p->p_fds[i] == -1) continue;

        chann_t *c = state.st_chann + p->p_fds[i];
        if (c->chann_mode == UNUSED) continue;

        kprintf("chann %d ino %d\n", c->chann_id,
                c->chann_vfile->vf_stat.st_ino);

        sprintf(buf, "%d", i);
        proc_fill_dirent(dir, c->chann_vfile->vf_stat.st_ino, buf);
        dir = proc_readdir(dir);
        // TODO problem if not same fs
        // TODO change type
    }

    proc_fill_dirent(dir, 0, "");

    return ino;
}

uint32_t proc_alloc_std_streams(pid_t pid) {
    char buf[256];
    sprintf(buf, "/proc/%d/fd", pid);
    vfile_t *vf = vfs_load(buf, 0);
    if (!vf) return 0;

    klogf(Log_info, "proc", "alloc streams /proc/%d/fd ino %d dev %d",
            pid, vf->vf_stat.st_ino, vf->vf_stat.st_dev);

    proc_t *p = state.st_proc + pid;

    vfile_t *stdin, *stdout, *stderr;
    cid_t cid_in, cid_out, cid_err;

    cid_in = free_chann();
    state.st_chann[cid_in].chann_mode = STREAM_IN;

    cid_out = free_chann();
    state.st_chann[cid_out].chann_mode = STREAM_OUT;

    cid_err = free_chann();
    state.st_chann[cid_err].chann_mode = STREAM_OUT;

    if (cid_in == NCHAN || cid_out == NCHAN || cid_out == NCHAN)
        return 0;

    stdin  = vfs_touch(buf, "0", 0640);
    stdout = vfs_touch(buf, "1", 0640);
    stderr = vfs_touch(buf, "2", 0640);
    if (!stdin || !stdout || !stderr) {
        state.st_chann[cid_in].chann_mode = UNUSED;
        state.st_chann[cid_out].chann_mode = UNUSED;
        state.st_chann[cid_err].chann_mode = UNUSED;
        vfs_close(stdin);
        vfs_close(stdout);
        vfs_close(stderr);
        return 0;
    }

    // stdin
    state.st_chann[cid_in].chann_vfile = stdin;
    state.st_chann[cid_in].chann_pos   = 0;
    state.st_chann[cid_in].chann_acc   = 1;
    p->p_fds[0] = cid_in;

    // stdout
    state.st_chann[cid_out].chann_vfile = stdout;
    state.st_chann[cid_out].chann_pos   = 0;
    state.st_chann[cid_out].chann_acc   = 1;
    p->p_fds[1] = cid_out;

    // stderr
    state.st_chann[cid_err].chann_vfile = stderr;
    state.st_chann[cid_err].chann_pos   = 0;
    state.st_chann[cid_err].chann_acc   = 1;
    p->p_fds[2] = cid_err;

    uint32_t ino = vf->vf_stat.st_ino;
    vfs_close(vf);
    return ino;
}
