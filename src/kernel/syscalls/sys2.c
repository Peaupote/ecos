#include <kernel/sys.h>

#include <stdint.h>

#include <libc/string.h>
#include <kernel/int.h>
#include <kernel/proc.h>
#include <kernel/kutil.h>
#include <kernel/memory/shared_pages.h>
#include <util/elf64.h>
#include <util/misc.h>
#include <fs/proc.h>

int sys_open(const char *fname, int oflags) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    cid_t cid;
    for (cid = 0; cid < NCHAN; cid++)
        if (state.st_chann[cid].chann_mode == UNUSED) break;

    if (cid == NCHAN) {
        klogf(Log_error, "sys", "no channel available");
        return -1;
    }

    chann_t *c = state.st_chann + cid;
    c->chann_vfile = vfs_load(fname, oflags);
    if (!c->chann_vfile) {
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
            klogf(Log_info, "syscall", "process %d open %s on %d (cid %d)",
                  state.st_curr_pid, fname, fd, cid);
            return fd;
        }
    }

    return -1;
}


int sys_close(int filedes) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    // invalid file descriptor
    if (filedes < 0 || filedes > NFD)
        return -1;

    // file descriptor reference no channel
    if (p->p_fds[filedes] == -1)
        return -1;

    klogf(Log_info, "syscall", "close filedes %d (cid %d)",
          filedes, p->p_fds[filedes]);

    chann_t *c = &state.st_chann[p->p_fds[filedes]];
    if (c->chann_mode != UNUSED && --c->chann_acc == 0) {
        vfs_close(c->chann_vfile);
        c->chann_mode = UNUSED;
    }

    p->p_fds[filedes] = -1;

    return 0;
}

int sys_dup(int fd) {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    int i;

    if (fd < 0 || fd > NFD || p->p_fds[fd] == -1)
        return -1;

    for (i = 0; i < NFD && p->p_fds[i] != -1; i++);

    if (i == NFD)
        // no file free descriptor
        return -1;

    p->p_fds[i] = p->p_fds[fd];
    ++state.st_chann[p->p_fds[fd]].chann_acc;
    klogf(Log_verb, "syscall", "dup %d on %d", fd, i);
    return i;
}

int sys_dup2(int fd1, int fd2) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    if (fd1 < 0 || fd2 < 0 || fd1 > NFD || fd2 > NFD || p->p_fds[fd1] == -1)
        return -1;

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
    if (fd[0] == NFD) return -1;

    for (fdout = fdin+1; fdout < NFD && p->p_fds[fdout] != -1; fdout++);
    if (fd[0] == NFD) return -1;

    cid_t in, out;
    for (in = 0; in < NCHAN && state.st_chann[in].chann_mode != UNUSED; in++);
    if (in == NCHAN) return -1;

    for (out = in; out < NCHAN && state.st_chann[out].chann_mode != UNUSED; out++);
    if (out == NCHAN) return -1;

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

    klogf(Log_error, "syscall", "alloc pipe, in %d, out %d", fdin, fdout);
    return 0;
}

ssize_t sys_read(int fd, uint8_t *d, size_t len) {
    proc_t *p  = &state.st_proc[state.st_curr_pid];;

    if (!d || fd < 0 || fd > NFD || p->p_fds[fd] == -1)
        return -1;

    cid_t cid = p->p_fds[fd];
    chann_t *chann = state.st_chann + cid;
    klogf(Log_verb, "syscall", "process %d read %d on %d (cid %d)",
          state.st_curr_pid, len, fd, cid);

    vfile_t *vfile = chann->chann_vfile;
    int rc;

    if (chann->chann_mode != READ && chann->chann_mode != RDWR)
        return -1;

    switch (vfile->vf_stat.st_mode&0xf000) {
    case TYPE_DIR:
    case TYPE_REG:
        if (chann->chann_pos == vfile->vf_stat.st_size) // EOF
            return 0;

        rc = vfs_read(vfile, d, chann->chann_pos, len);
        if (rc > 0) chann->chann_pos += rc;
        klogf(Log_verb, "syscall", "%d char readed (pos %d)",
              rc, chann->chann_pos);
        return rc;

    case TYPE_CHAR:
    case TYPE_FIFO:
        if (vfile->vf_stat.st_size == 0) {
            wait_file(state.st_curr_pid, cid);
        }

        return vfs_read(vfile, d, 0, len);
        break;

    default:
        return -1;
    }
}

ssize_t sys_write(int fd, uint8_t *s, size_t len) {
    proc_t *p  = &state.st_proc[state.st_curr_pid];;

    if (!s || fd < 0 || fd > NFD || p->p_fds[fd] == -1) {
        klogf(Log_error, "syscall", "write to bad file descriptor %d", fd);
        return -1;
    }

    chann_t *chann = &state.st_chann[p->p_fds[fd]];
    vfile_t *vfile = chann->chann_vfile;
    klogf(Log_verb, "syscall", "process %d write on %d",
            state.st_curr_pid, fd);

    size_t c = 0;
    int rc = 0;

    if (chann->chann_mode != WRITE && chann->chann_mode != RDWR) {
        klogf(Log_info, "syscall", "can't write on chann %d\n", p->p_fds[fd]);
        return -1;
    }

    switch (vfile->vf_stat.st_mode&0xf000) {
    case TYPE_REG:
        rc = vfs_write(vfile, s, chann->chann_pos, len);
        if (rc > 0) chann->chann_pos += rc;
        return rc;

    case TYPE_FIFO:
        return vfs_write(vfile, s, 0, len);

    case TYPE_CHAR:
        // TODO : more efficient
        for (size_t i = 0; i < len; i++)
            kprintf("%c", *s++);

        return c;

    default:
        return -1;
    }
}

off_t sys_lseek(int fd, off_t off) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    if (fd < 0 || fd > NFD) {
        klogf(Log_error, "syscall",
                "process %d lseek on invalid file descriptor %d",
                state.st_curr_pid, fd);
        return -1;
    }

    klogf(Log_verb, "syscall", "process %d lseek %d at %d",
            state.st_curr_pid, fd, off);

    state.st_chann[p->p_fds[fd]].chann_pos = off;

    return off;
}

int sys_fstat(int fd, struct stat *st) {
    proc_t *p = state.st_proc + state.st_curr_pid;

    if (fd < 0 || fd > NFD) return -1;
    if (p->p_fds[fd] == -1) return -1;

    chann_t *c = state.st_chann + p->p_fds[fd];
    if (c->chann_mode == UNUSED) return -1;

    memcpy(st, &c->chann_vfile->vf_stat, sizeof (struct stat));
    return 0;
}
