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
#include <fs/proc.h>

void sys_errno(int *errno) {
    state.st_proc[state.st_curr_pid].p_errno = errno;
}

int sys_open(const char *fname, int oflags) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    cid_t cid;
    for (cid = 0; cid < NCHAN; cid++)
        if (state.st_chann[cid].chann_mode == UNUSED) break;

    if (cid == NCHAN) {
        klogf(Log_error, "sys", "no channel available");
        set_errno(p, ENFILE);
        return -1;
    }

    chann_t *c = state.st_chann + cid;
    c->chann_vfile = vfs_load(fname, oflags);
    if (!c->chann_vfile) {
        set_errno(p, ENOENT);
        klogf(Log_error, "sys", "process %d couldn't open %s",
                state.st_curr_pid, fname);
        return -1;
    }

    for (int fd = 0; fd < NFD; fd++) {
        if (p->p_fds[fd] == -1) {
            c->chann_acc  = 1;
            c->chann_mode = oflags&3;
            c->chann_pos  = 0;
            p->p_fds[fd] = cid;
            set_errno(p, SUCC);
            klogf(Log_info, "syscall", "process %d open %s on %d (cid %d)",
                  state.st_curr_pid, fname, fd, cid);
            return fd;
        }
    }

    set_errno(p, EMFILE);

    return -1;
}


int sys_close(int filedes) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    // invalid file descriptor
    if (filedes < 0 || filedes > NFD || p->p_fds[filedes] == -1) {
        set_errno(p, EBADF);
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
    set_errno(p, SUCC);

    return 0;
}

int sys_dup(int fd) {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    int i;

    if (fd < 0 || fd > NFD || p->p_fds[fd] == -1) {
        set_errno(p, EBADF);
        return -1;
    }

    for (i = 0; i < NFD && p->p_fds[i] != -1; i++);

    if (i == NFD) {
        set_errno(p, EMFILE);
        return -1;
    }

    p->p_fds[i] = p->p_fds[fd];
    ++state.st_chann[p->p_fds[fd]].chann_acc;
    klogf(Log_verb, "syscall", "dup %d on %d", fd, i);
    set_errno(p, SUCC);
    return i;
}

int sys_dup2(int fd1, int fd2) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    if (fd1 < 0 || fd2 < 0 || fd1 > NFD || fd2 > NFD || p->p_fds[fd1] == -1) {
        set_errno(p, EBADF);
        return -1;
    }

    p->p_fds[fd2] = p->p_fds[fd1];
    ++state.st_chann[p->p_fds[fd1]].chann_acc;
    klogf(Log_error, "syscall", "dup %d on %d", fd1, fd2); // <<=
    return fd2;
}

int sys_pipe(int fd[2]) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    if (!fd) return -1;

    int fdin, fdout;

    for (fdin = 0; fdin < NFD && p->p_fds[fdin] != -1; fdin++);
    if (fdin == NFD) goto err_emfile;

    for (fdout = fdin+1; fdout < NFD && p->p_fds[fdout] != -1; fdout++);
    if (fdout == NFD) goto err_emfile;

    cid_t in, out;
    for (in = 0; in < NCHAN && state.st_chann[in].chann_mode != UNUSED; in++);
    if (in == NCHAN) goto err_enfile;

    for (out = in; out < NCHAN && state.st_chann[out].chann_mode != UNUSED; out++);
    if (out == NCHAN) goto err_enfile;

    chann_t *cin = &state.st_chann[in],
        *cout = &state.st_chann[out];

    fd[0] = fdin;
    fd[1] = fdout;
    p->p_fds[fdin] = in;
    p->p_fds[fdout] = out;

    cin->chann_acc   = 1;
    cin->chann_mode  = READ;
    cout->chann_acc  = 1;
    cout->chann_mode = WRITE;

    vfile_t *pipe = vfs_pipe();
    cin->chann_vfile  = pipe;
    cout->chann_vfile = pipe;

    set_errno(p, SUCC);
    klogf(Log_error, "syscall", "alloc pipe, in %d, out %d", fdin, fdout);
    return 0;

err_emfile:
    set_errno(p, EMFILE);
    return -1;

err_enfile:
    set_errno(p, ENFILE);
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
    case TYPE_REG:
        if (chann->chann_pos == vfile->vf_stat.st_size)
            goto succ;

        rc = vfs_read(vfile, d, chann->chann_pos, len);
        if (rc < 0)
            goto err_overflow; // TODO correct

        chann->chann_pos += rc;
        klogf(Log_verb, "syscall", "%d char readed (pos %d)",
              rc, chann->chann_pos);
        goto succ;

    case TYPE_CHAR:
    case TYPE_FIFO:
        if (vfile->vf_stat.st_size == 0) {
            wait_file(state.st_curr_pid, cid);
        }

        rc = vfs_read(vfile, d, 0, len);
        goto succ;

    default:
        goto err_badf;
    }

succ:
    set_errno(p, SUCC);
    return rc;

err_overflow:
    set_errno(p, EOVERFLOW);
    return -1;

err_badf:
    set_errno(p, EBADF);
    return -1;
}

ssize_t sys_write(int fd, uint8_t *s, size_t len) {
    proc_t *p  = &state.st_proc[state.st_curr_pid];;

    if (!s || fd < 0 || fd > NFD || p->p_fds[fd] == -1)
        goto err_badf;

    chann_t *chann = &state.st_chann[p->p_fds[fd]];
    vfile_t *vfile = chann->chann_vfile;
    klogf(Log_verb, "syscall", "process %d write on %d",
            state.st_curr_pid, fd);

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
        rc = vfs_write(vfile, s, 0, len);
        goto succ;

    case TYPE_CHAR:
        // TODO : more efficient
        for (rc = 0; rc < (int)len; ++rc)
            kprintf("%c", *s++);
        goto succ;

    default:
        goto err_badf;
    }

succ:
    set_errno(p, SUCC);
    return rc;

err_overflow:
    set_errno(p, EOVERFLOW);
    return -1;

err_badf:
    set_errno(p, EBADF);
    return -1;
}

off_t sys_lseek(int fd, off_t off, int whence) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    klogf(Log_verb, "syscall", "lseek on %d off %d whence %d",
          fd, off, whence);

    if (fd < 0 || fd > NFD || p->p_fds[fd] == -1) {
        set_errno(p, EBADF);
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
    set_errno(p, SUCC);
    return off;

err_einval:
    set_errno(p, EINVAL);
    return -1;
}

int sys_fstat(int fd, struct stat *st) {
    proc_t *p = state.st_proc + state.st_curr_pid;

    if (fd < 0 || fd > NFD || p->p_fds[fd] == -1) goto err_badf;

    chann_t *c = state.st_chann + p->p_fds[fd];
    if (c->chann_mode == UNUSED) goto err_badf;

    memcpy(st, &c->chann_vfile->vf_stat, sizeof (struct stat));
    set_errno(p, SUCC);
    return 0;

err_badf:
    set_errno(p, EBADF);
    return -1;
}
