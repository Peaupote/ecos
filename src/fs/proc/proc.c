#include <headers/unistd.h>

#include <kernel/file.h>
#include <kernel/proc.h>

#include <fs/proc.h>
#include <fs/pipe.h>

#include <libc/string.h>

#include "special.h"

int proc_mount(void *partition __attribute__((unused)),
               struct mount_info *info) {
    for (size_t i = 0; i < NPROC; i++) {
        proc_inodes[i].st.st_size  = 0;
        proc_inodes[i].st.st_nlink = 0;
    }

    memset(&proc_block_bitmap, 0, PROC_BITMAP_SIZE);

    // irrelevant here
    info->sp = 0;

    info->block_size = PROC_BLOCK_SIZE;
    info->root_ino   = PROC_ROOT_INO;
    proc_init_dir(PROC_ROOT_INO, PROC_ROOT_INO, TYPE_DIR|040);
    proc_mkdir(PROC_ROOT_INO, "pipes", TYPE_DIR|0400, info);
    ino_t tty = proc_mkdir(PROC_ROOT_INO, "tty", TYPE_DIR|0400, info);

    proc_alloc_pipe(tty, "tty0", TYPE_CHAR|0400);
    proc_alloc_pipe(tty, "tty1", TYPE_CHAR|0400);
    proc_alloc_pipe(tty, "tty2", TYPE_CHAR|0400);

    return 1;
}

int proc_stat(ino_t ino, struct stat *st,
              struct mount_info *info __attribute__((unused))) {
    memcpy(st, &proc_inodes[ino].st, sizeof(struct stat));
    st->st_ino = ino;
    return ino;
}

int proc_read(ino_t ino, void *buf, off_t pos __attribute__((unused)),
              size_t len, struct mount_info *info __attribute__((unused))) {
    struct proc_inode *inode = proc_inodes + ino;
    int rc;
    size_t i;
    char *src, *dst;
    struct pipe_inode *pipe;
    proc_op_t *read;
    proc_st_t *stat;
    pid_t pid;
    vfile_t *vf;

    switch (inode->st.st_mode&0xf000) {
    case TYPE_SYM:
        dst = (char*)inode->in_block;
        vf = vfs_load(dst, 0);
        if (!vf) {
            klogf(Log_error, "procfs", "dangling reference %s", dst);
            return -1;
        }

        rc = vfs_read(vf, buf, pos, len);
        vfs_close(vf);
        return rc;

    case TYPE_DIR:
        src = (char*)inode->in_block[0] + pos;
        dst = (char*)buf;
        for (i = 0; i < len; i++)
            *dst++ = *src++;
        return i;

    case TYPE_REG:
        pid   = (pid_t)inode->in_block[SPEC_PID];
        read = (proc_op_t*)inode->in_block[SPEC_READ];
        stat = (proc_st_t*)inode->in_block[SPEC_STAT];
        rc = read(pid, buf, pos, len);
        stat(pid, ino, &inode->st);
        return rc;

    case TYPE_CHAR:
    case TYPE_FIFO:
        pipe = (struct pipe_inode*)inode->in_block[0];
        rc = pipe_read(pipe, buf, len);
        pipe_stat(pipe, &inode->st);
        return rc;

    default:
        kAssert(false);
        return 0;
    }
}

int proc_write(ino_t ino, void *buf, off_t pos __attribute__((unused)),
              size_t len, struct mount_info *info __attribute__((unused))) {
    struct proc_inode *inode = proc_inodes + ino;
    struct pipe_inode *pipe;
    int rc;
    char *dst;
    vfile_t *vf;

    switch (inode->st.st_mode&0xf000) {
    case TYPE_SYM:
        dst = (char*)inode->in_block;
        vf = vfs_load(dst, 0);
        if (!vf) {
            klogf(Log_error, "procfs", "dangling reference %s", dst);
            return -1;
        }

        rc = vfs_write(vf, buf, pos, len);
        vfs_close(vf);
        return rc;

    case TYPE_CHAR:
    case TYPE_FIFO:
        pipe = (struct pipe_inode*)inode->in_block[0];
        rc = pipe_write(pipe, buf, len);
        pipe_stat(pipe, &inode->st);
        return rc;

    default:
        kAssert(false);
        return 0;
    }
}

uint32_t proc_alloc_pipe(ino_t parent, const char *fname, uint16_t type) {

    // dont create directories with touch
    if (type&TYPE_DIR) {
        klogf(Log_error, "procfs", "can't create directory");
        return 0;
    }

    struct proc_inode *p = proc_inodes + parent;
    if (!(p->st.st_mode&TYPE_DIR)) {
        klogf(Log_error, "procfs", "invalid ino %d is not a directory", parent);
        return 0;
    }

    uint32_t ino;
    if (!(ino = proc_free_inode())) {
        klogf(Log_error, "procfs", "no free inodes");
        return 0;
    }

    struct proc_inode *inode = proc_inodes + ino;

    memset(inode, 0, sizeof(struct proc_inode));
    inode->in_block[0] = (uint64_t)pipe_alloc();
    inode->st.st_mode = type;

    if (!proc_add_dirent(p, ino, fname)) {
        klogf(Log_error, "procfs", "failed to add dir entry");
        return 0;
    }

    return ino;
}

uint32_t proc_touch(ino_t parent, const char *fname, uint16_t type,
                    struct mount_info *info) {
    uint32_t ino;

    // test if file already exists and is not a directory
    if ((ino = proc_lookup(fname, parent, info)) &&
        !(proc_inodes[ino].st.st_mode&TYPE_DIR)) {
        klogf(Log_error, "procfs", "file already exists");
        return 0;
    }

    // dont create directories with touch
    if (type&TYPE_DIR) {
        klogf(Log_error, "procfs", "can't create directory with touch");
        return 0;
    }

    struct proc_inode *p = proc_inodes + parent;
    if (!(p->st.st_mode&TYPE_DIR)) {
        klogf(Log_error, "procfs", "invalid ino %d is not a directory", parent);
        return 0;
    }

    if (!(ino = proc_free_inode())) {
        klogf(Log_error, "procfs", "no free inodes");
        return 0;
    }

    struct proc_inode *inode = proc_inodes + ino;

    uint32_t b = proc_free_block();
    if (b == PROC_NBLOCK) {
        klogf(Log_error, "procfs", "no free block");
        return 0;
    }

    inode->in_block[0] = (uint64_t)(proc_blocks + b);
    inode->st.st_nlink = 0;

    if (!proc_add_dirent(p, ino, fname)) {
        klogf(Log_error, "procfs", "failed to add dir entry");
        return 0;
    }

    inode->st.st_mode  = type;
    inode->st.st_uid   = 0;
    inode->st.st_size  = 0;
    inode->st.st_ctime = 0; // TODO now
    inode->st.st_mtime = 0;
    inode->st.st_gid   = 0;

    // set block b to used
    proc_block_bitmap[b >> 3] |= 1 << (b&7);

    klogf(Log_info, "procfs", "touch %s inode %d", fname, ino);

    return ino;
}


ino_t proc_readsymlink(ino_t ino, char *dst,
                       struct mount_info *info __attribute__((unused))) {
    struct proc_inode *inode = proc_inodes + ino;
    if (!(inode->st.st_mode&TYPE_SYM)) return 0;

    strcpy(dst, (char*)&inode->in_block);
    return ino;
}

uint32_t proc_create(pid_t pid) {
    struct device *dev = find_device("/proc");
    char buf[256] = { 0 };
    uint32_t ino, rc;

    klogf(Log_info, "procfs", "proc create /proc/%d", pid);

    sprintf(buf, "%d", pid);
    ino = proc_mkdir(PROC_ROOT_INO, buf,
                     TYPE_DIR|0600, &dev->dev_info);
    if (!ino) {
        klogf(Log_error, "procfs", "failed to mkdir /proc/%d", pid);
        return 0;
    }

    struct proc_inode *proc = proc_inodes + ino;
    struct dirent *dir = proc_opendir(ino, &dev->dev_info);

    // skip . and ..
    for (size_t l = 0; l < proc->st.st_size;
         l += dir->d_rec_len, dir = proc_readdir(dir));

    ino_t st_ino = alloc_special_file(pid, stat_read, stat_stat);
    proc_fill_dirent(dir, st_ino, "stat");
    proc->st.st_size += dir->d_rec_len;
    dir = proc_readdir(dir);

    ino_t st_cmd = alloc_special_file(pid, cmd_read, cmd_stat);
    proc_fill_dirent(dir, st_cmd, "cmd");
    proc->st.st_size += dir->d_rec_len;
    dir = proc_readdir(dir);

    proc_fill_dirent(dir, 0, "");

    rc = proc_mkdir(ino, "fd", TYPE_DIR|0600, &dev->dev_info);
    if (!rc) {
        klogf(Log_error, "procfs", "failed to create /proc/%d/fd", pid);
        return 0;
    }

    proc_t *p = state.st_proc + pid;
    dir = proc_opendir(rc, &dev->dev_info);
    struct proc_inode *fd = proc_inodes + rc;

    // skip . and ..
    for (size_t l = 0; l < fd->st.st_size;
         l += dir->d_rec_len, dir = proc_readdir(dir));

    for (int i = 0; i < NFD; i++) {
        if (p->p_fds[i] == -1) continue;

        chann_t *c = state.st_chann + p->p_fds[i];
        if (c->chann_mode == UNUSED) continue;

        rc = sprintf(buf, "%d", i);
        buf[rc] = 0;
        proc_fill_dirent(dir, c->chann_vfile->vf_stat.st_ino, buf);
        fd->st.st_size += dir->d_rec_len;

        proc_inodes[c->chann_vfile->vf_stat.st_ino].st.st_nlink++;
        dir = proc_readdir(dir);

        // TODO problem if not same fs
        // TODO change type
    }

    proc_fill_dirent(dir, 0, "");

    return ino;
}

uint32_t proc_alloc_std_streams(pid_t pid) {
    char buf[256] = { 0 };
    sprintf(buf, "/proc/%d/fd", pid);
    vfile_t *vf = vfs_load(buf, 0);
    if (!vf) return 0;

    klogf(Log_info, "proc", "alloc streams /proc/%d/fd ino %d dev %d",
            pid, vf->vf_stat.st_ino, vf->vf_stat.st_dev);

    proc_t *p = state.st_proc + pid;

    ino_t stdin, stdout, stderr;
    cid_t cid_in, cid_out, cid_err;

    cid_in = free_chann();
    state.st_chann[cid_in].chann_mode     = READ;
    state.st_chann[cid_in].chann_waiting  = PID_NONE;
    state.st_chann[cid_in].chann_nxw      = cid_in;

    cid_out = free_chann();
    state.st_chann[cid_out].chann_mode    = WRITE;
    state.st_chann[cid_out].chann_waiting = PID_NONE;
    state.st_chann[cid_out].chann_nxw     = cid_out;

    cid_err = free_chann();
    state.st_chann[cid_err].chann_mode    = WRITE;
    state.st_chann[cid_err].chann_waiting = PID_NONE;
    state.st_chann[cid_err].chann_nxw     = cid_err;

    if (cid_in == NCHAN || cid_out == NCHAN || cid_out == NCHAN) {
        vfs_close(vf);
        return 0;
    }

    stdin  = proc_free_inode();
    proc_inodes[stdin].st.st_nlink = 1;

    stdout = proc_free_inode();
    proc_inodes[stdout].st.st_nlink = 1;

    stderr = proc_free_inode();
    proc_inodes[stderr].st.st_nlink = 1;

    // add dirent increment number of link
    // inodes are unused of fail
    proc_inodes[stdin].st.st_nlink  = 0;
    proc_inodes[stdout].st.st_nlink = 0;
    proc_inodes[stderr].st.st_nlink = 0;

    if (!stdin || !stdout || !stderr) {
        state.st_chann[cid_in].chann_mode = UNUSED;
        state.st_chann[cid_out].chann_mode = UNUSED;
        state.st_chann[cid_err].chann_mode = UNUSED;
        vfs_close(vf);
        return 0;
    }

    struct proc_inode *inode;

    const char *ptty0  = "/proc/tty/tty0";
    const char *ptty1  = "/proc/tty/tty1";
    vfile_t *tty0  = vfs_load(ptty0, 0);
    vfile_t *tty1  = vfs_load(ptty1, 0);

    // stdin
    inode = proc_inodes + stdin;
    inode->st.st_mode = TYPE_SYM;
    memcpy(&inode->in_block, ptty0, 15);
    if (!proc_add_dirent(proc_inodes + vf->vf_stat.st_ino, stdin, "0")) {
        klogf(Log_error, "procfs", "failed to add dir entry");
        return 0;
    }

    state.st_chann[cid_in].chann_vfile = tty0;
    state.st_chann[cid_in].chann_mode  = READ;
    state.st_chann[cid_in].chann_pos   = 0;
    state.st_chann[cid_in].chann_acc   = 1;
    p->p_fds[STDIN_FILENO] = cid_in;

    // stdout
    inode = proc_inodes + stdout;
    inode->st.st_mode = TYPE_SYM;
    memcpy(&inode->in_block, ptty1, 15);
    if (!proc_add_dirent(proc_inodes + vf->vf_stat.st_ino, stdout, "1")) {
        klogf(Log_error, "procfs", "failed to add dir entry");
        return 0;
    }

    state.st_chann[cid_out].chann_vfile = tty1;
    state.st_chann[cid_out].chann_mode  = WRITE;
    state.st_chann[cid_out].chann_pos   = 0;
    state.st_chann[cid_out].chann_acc   = 1;
    p->p_fds[STDOUT_FILENO] = cid_out;

    // stderr
    inode = proc_inodes + stderr;
    inode->st.st_mode = TYPE_SYM;
    memcpy(&inode->in_block, ptty1, 15);
    if (!proc_add_dirent(proc_inodes + vf->vf_stat.st_ino, stderr, "2")) {
        klogf(Log_error, "procfs", "failed to add dir entry");
        return 0;
    }

    state.st_chann[cid_err].chann_vfile = tty1;
    state.st_chann[cid_err].chann_mode  = WRITE;
    state.st_chann[cid_err].chann_pos   = 0;
    state.st_chann[cid_err].chann_acc   = 1;
    p->p_fds[STDERR_FILENO] = cid_err;

    uint32_t ino = vf->vf_stat.st_ino;
    vfs_close(vf);
    return ino;
}

ino_t proc_rm(ino_t ino, struct mount_info *info __attribute__((unused))) {
    struct proc_inode *inode = proc_inodes + ino;
    if (--inode->st.st_nlink > 0) return ino;

    klogf(Log_info, "procfs", "free inode %d", ino);
    uint32_t b;

    switch (inode->st.st_mode&0xf000) {
    case TYPE_REG: // nothing todo
        break;

    case TYPE_DIR:
        b = inode->in_block[0];
        proc_block_bitmap[b >> 3] &= ~(1 << (b&7));
        break;

    case TYPE_CHAR:
    case TYPE_FIFO:
        pipe_free((struct pipe_inode*)inode->in_block[0]);
        break;

    default:
        return 0;
    }

    return ino;
}

uint32_t proc_exit(pid_t pid) {
    char fname[256] = { 0 };

    klogf(Log_info, "procfs", "remove dir /proc/%d", pid);
    sprintf(fname, "/proc/%d", pid);
    return vfs_rmdir(fname, 1);
}
