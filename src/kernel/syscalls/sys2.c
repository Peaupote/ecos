#include <kernel/sys.h>

#include <stdint.h>

#include <libc/errno.h>
#include <libc/string.h>
#include <kernel/int.h>
#include <kernel/proc.h>
#include <kernel/kutil.h>
#include <kernel/memory/shared_pages.h>
#include <util/elf64.h>
#include <util/misc.h>

void sys_errno(int *errno) {
    state.st_proc[state.st_curr_pid].p_errno = errno;
}

int sys_open(const char *fname, int oflags, int perms) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    cid_t cid;
    for (cid = 0; cid < NCHAN; cid++)
        if (state.st_chann[cid].chann_mode == UNUSED) break;

    if (cid == NCHAN) {
        klogf(Log_info, "sys", "no channel available");
        set_errno(ENFILE);
        return -1;
    }

    int is_new_file = 0;
    chann_t *c = state.st_chann + cid;
    c->chann_vfile = vfs_load(fname, oflags);
    if (!c->chann_vfile) {
        if (oflags&O_CREAT && vfs_create(fname, TYPE_REG|perms)) {
            c->chann_vfile = vfs_load(fname, oflags);
            is_new_file   = 1;
            if (!c->chann_vfile) {
                set_errno(ENOENT);
                klogf(Log_info, "syscall", "couldn't create file %s", fname);
                return -1;
            }
        } else {
            set_errno(ENOENT);
            klogf(Log_info, "sys", "process %d couldn't open %s",
                  state.st_curr_pid, fname);
            return -1;
        }
    } else if (oflags & O_TRUNC) {
        vfs_truncate(c->chann_vfile);
    }

    for (int fd = 0; fd < NFD; fd++) {
        if (p->p_fds[fd] == -1) {
            c->chann_acc  = 1;
            c->chann_mode = oflags&3;
            c->chann_pos  = 0;
            c->chann_nxw  = cid;
            c->chann_waiting = PID_NONE;
            vfs_opench(c->chann_vfile, &c->chann_adt);
            p->p_fds[fd] = cid;

            if (oflags&O_APPEND) {
                klogf(Log_info, "syscall", "flag O_APPEND is on for %d", fd);
                int rc = sys_lseek(fd, 0, SEEK_END);
                if (rc < 0) return rc;
                return fd;
            }

            set_errno(SUCC);
            klogf(Log_info, "syscall", "process %d open %s on %d (cid %d)",
                  state.st_curr_pid, fname, fd, cid);
            return fd;
        }
    }

    // if file was juste created, clean the mess
    if (is_new_file) vfs_rm(fname);
    set_errno(EMFILE);

    return -1;
}


int sys_close(int filedes) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    // invalid file descriptor
    if (filedes < 0 || filedes > NFD || p->p_fds[filedes] == -1) {
        set_errno(EBADF);
        return -1;
    }

    klogf(Log_info, "syscall", "close filedes %d (cid %d)",
          filedes, p->p_fds[filedes]);

    chann_t *c = &state.st_chann[p->p_fds[filedes]];
    if (c->chann_mode != UNUSED && --c->chann_acc == 0) {
        vfs_close(c->chann_vfile);
        c->chann_mode = UNUSED;
    }

    p->p_fds[filedes] = -1;
    set_errno(SUCC);

    return 0;
}

int sys_dup(int fd) {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    int i;

    if (fd < 0 || fd > NFD || p->p_fds[fd] == -1) {
        set_errno(EBADF);
        return -1;
    }

    for (i = 0; i < NFD && p->p_fds[i] != -1; i++);

    if (i == NFD) {
        set_errno(EMFILE);
        return -1;
    }

    p->p_fds[i] = p->p_fds[fd];
    ++state.st_chann[p->p_fds[fd]].chann_acc;
    klogf(Log_verb, "syscall", "dup %d on %d", fd, i);
    set_errno(SUCC);
    return i;
}

int sys_dup2(int fd1, int fd2) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    if (fd1 < 0 || fd2 < 0 || fd1 > NFD || fd2 > NFD || p->p_fds[fd1] == -1) {
        set_errno(EBADF);
        return -1;
    }

    p->p_fds[fd2] = p->p_fds[fd1];
    ++state.st_chann[p->p_fds[fd1]].chann_acc;
    klogf(Log_info, "syscall", "dup %d on %d", fd1, fd2); // <<=
    return fd2;
}

int sys_pipe(int fd[2]) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    if (!fd) return -1;

    int fdrd, fdwt;

    for (fdrd = 0; fdrd < NFD && ~p->p_fds[fdrd]; fdrd++);
    if (fdrd == NFD) goto err_emfile;

    for (fdwt = fdrd+1; fdwt < NFD && ~p->p_fds[fdwt]; fdwt++);
    if (fdwt == NFD) goto err_emfile;

    cid_t rd, wt;
    for (rd = 0; rd < NCHAN && state.st_chann[rd].chann_mode != UNUSED; rd++);
    if (rd == NCHAN) goto err_enfile;

    for (wt = rd + 1;
            wt < NCHAN && state.st_chann[wt].chann_mode != UNUSED; wt++);
    if (wt == NCHAN) goto err_enfile;

    chann_t *crd = &state.st_chann[rd],
            *cwt = &state.st_chann[wt];

    fd[0] = fdrd;
    fd[1] = fdwt;
    p->p_fds[fdrd] = rd;
    p->p_fds[fdwt] = wt;

    crd->chann_acc  = 1;
    crd->chann_mode = READ;
    cwt->chann_acc  = 1;
    cwt->chann_mode = WRITE;

    vfile_t* fvf[2];
    if (!~vfs_pipe(fvf))
        return -1; //TODO errno
    crd->chann_vfile = fvf[0];
    cwt->chann_vfile = fvf[1];
    kAssert(fvf[0] != NULL);
    kAssert(fvf[1] != NULL);

    set_errno(SUCC);
    klogf(Log_info, "syscall", "alloc pipe, read %d(%d), write %d(%d)",
            fdrd, rd, fdwt, wt);
    return 0;

err_emfile:
    set_errno(EMFILE);
    return -1;

err_enfile:
    set_errno(ENFILE);
    return -1;
}

ssize_t sys_read(int fd, uint8_t *d, size_t len) {
    proc_t *p  = &state.st_proc[state.st_curr_pid];;

    if (!d || fd < 0 || fd > NFD || p->p_fds[fd] == -1)
        goto err_badf;

    cid_t cid = p->p_fds[fd];
    chann_t *chann = state.st_chann + cid;
    klogf(Log_verb, "syscall", "process %d read %d on %d (cid %d)",
          state.st_curr_pid, len, fd, cid);

    vfile_t *vfile = chann->chann_vfile;
    int rc = 0;

    if (chann->chann_mode != READ && chann->chann_mode != RDWR)
        goto err_badf;

    switch (vfile->vf_stat.st_mode&0xf000) {
    case TYPE_DIR:
        rc = vfs_getdents(vfile, (struct dirent*)d, len,
                &chann->chann_adt);
        if (rc < 0) goto err_overflow; //TODO
        goto succ;
    case TYPE_REG:
        if (chann->chann_pos >= vfile->vf_stat.st_size) {
            chann->chann_pos = vfile->vf_stat.st_size;
            goto succ;
        }

        rc = vfs_read(vfile, d, chann->chann_pos, len);
        if (rc < 0)
            goto err_overflow; // TODO correct

        chann->chann_pos += rc;
        klogf(Log_verb, "syscall", "%d char readed (pos %d)",
              rc, chann->chann_pos);
        goto succ;

    case TYPE_CHAR:
    case TYPE_FIFO:
        if (vfile->vf_stat.st_size == 0)
            wait_file(state.st_curr_pid, cid);

        rc = vfs_read(vfile, d, 0, len);
        if (rc == -2)
            wait_file(state.st_curr_pid, cid);
        else if (rc < 0)
            goto err_overflow; //TODO

        goto succ;

    default:
        goto err_badf;
    }

succ:
    set_errno(SUCC);
    return rc;

err_overflow:
    set_errno(EOVERFLOW);
    return -1;

err_badf:
    set_errno(EBADF);
    return -1;
}

ssize_t sys_write(int fd, uint8_t *s, size_t len) {
    proc_t *p  = &state.st_proc[state.st_curr_pid];;

    if (!s || fd < 0 || fd > NFD || p->p_fds[fd] == -1)
        goto err_badf;

    cid_t cid = p->p_fds[fd];
    chann_t *chann = &state.st_chann[cid];
    vfile_t *vfile = chann->chann_vfile;
    klogf(Log_verb, "syscall", "process %d write on %d(%d -> %d@%d)",
          state.st_curr_pid, fd, p->p_fds[fd],
          vfile->vf_stat.st_ino, vfile->vf_stat.st_dev);

    int rc = 0;

    if (chann->chann_mode != WRITE && chann->chann_mode != RDWR)
        goto err_badf;

    switch (vfile->vf_stat.st_mode&0xf000) {
    case TYPE_REG:
        rc = vfs_write(vfile, s, chann->chann_pos, len);
        if (rc < 0) goto err_overflow; // TODO correct
        chann->chann_pos += rc;
        goto succ;

    case TYPE_FIFO:
    case TYPE_CHAR:
        rc = vfs_write(vfile, s, 0, len);
        if (rc == -2)
            wait_file(state.st_curr_pid, cid);
        else if (rc < 0)
            goto err_overflow; //TODO
        goto succ;

    default:
        goto err_badf;
    }

succ:
    set_errno(SUCC);
    return rc;

err_overflow:
    set_errno(EOVERFLOW);
    return -1;

err_badf:
    set_errno(EBADF);
    return -1;
}

off_t sys_lseek(int fd, off_t off, int whence) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    klogf(Log_verb, "syscall", "lseek on %d off %d whence %d",
          fd, off, whence);

    if (fd < 0 || fd > NFD || p->p_fds[fd] == -1) {
        set_errno(EBADF);
        return -1;
    }

    chann_t *c = state.st_chann + p->p_fds[fd];
    off_t pos;

    switch (whence) {
    case SEEK_SET:
        pos = off;
        break;

    case SEEK_CUR:
        pos = off + c->chann_pos;
        break;

    case SEEK_END:
        pos = off + c->chann_vfile->vf_stat.st_size;
        break;

    default:
        goto err_einval;
    }

    if (pos < 0 || (size_t)pos > c->chann_vfile->vf_stat.st_size)
        goto err_einval;

    c->chann_pos = pos;
    set_errno(SUCC);
    return off;

err_einval:
    set_errno(EINVAL);
    return -1;
}

int sys_fstat(int fd, struct stat *st) {
    proc_t *p = cur_proc();

    if (fd < 0 || fd > NFD || p->p_fds[fd] == -1) goto err_badf;

    chann_t *c = state.st_chann + p->p_fds[fd];
    if (c->chann_mode == UNUSED) goto err_badf;

    memcpy(st, &c->chann_vfile->vf_stat, sizeof (struct stat));
    set_errno(SUCC);
    return 0;

err_badf:
    set_errno(EBADF);
    return -1;
}

int sys_mkdir(const char *path, mode_t mode) {
    if (!path) return 0;

    uint32_t ino = vfs_create(path, TYPE_DIR|mode);
    set_errno(SUCC);
    return ino ? 0 : -1;
}

int sys_chdir(const char *fname) {
    if (!fname) {
        set_errno(EINVAL);
        return -1;
    }

    proc_t *p    = cur_proc();
    ino_t ino    = p->p_cino;
    dev_t dev_id = p->p_dev;

    if (!vfs_find(fname, fname + strlen(fname), &dev_id, &ino)) {
        set_errno(ENOENT);
        return -1;
    }

    struct stat st;
    struct device *dev = devices + dev_id;
    fst[dev->dev_fs].fs_stat(ino, &st, &dev->dev_info);
    if (!(st.st_mode & TYPE_DIR)) {
        set_errno(ENOTDIR);
        return -1;
    }

    p->p_cino = ino;
    p->p_dev  = dev_id;

    return 0;
}
