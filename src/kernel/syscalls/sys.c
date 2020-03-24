#include <stdint.h>

#include <kernel/int.h>
#include <kernel/proc.h>
#include <kernel/kutil.h>
#include <kernel/sys.h>
#include <kernel/memory/shared_pages.h>
#include <util/elf64.h>

/**
 * Syscalls
 */

void kexit() {
    proc_t *p = &state.st_proc[state.st_curr_pid],
        *pp = &state.st_proc[p->p_ppid];

    int status = p->p_reg.rdi;
    klogf(Log_info, "syscall",
          "kill pid %d with status %d", p->p_pid, status);

    klogf(Log_verb, "mem", "pre_free: %lld pages disponibles",
            (long long int)kmem_nb_page_free());
    kmem_free_paging(p->p_pml4, kernel_pml4);

    if (pp->p_stat == WAIT) {
        pp->p_stat    = RUN;
        kAssert(!push_ps(p->p_ppid));
        pp->p_reg.rax = status;
        pp->p_nchd--; // one child less
        p->p_ppid = 0;
        p->p_stat = FREE;
    } else
        p->p_stat = ZOMB;

    for (pid_t pid = 2; pid < NPROC; pid++) {
        pp = &state.st_proc[pid];
        if (pp->p_ppid == state.st_curr_pid) {
            pp->p_ppid = 1;
            state.st_proc[1].p_nchd++;
        }
    }

    schedule_proc(1);
    kAssert(false);
}

pid_t getpid() {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    klogf(Log_verb, "syscall", "getpid %d", p->p_pid);
    return p->p_pid;
}

pid_t getppid() {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    klogf(Log_verb, "syscall", "getppid %d", p->p_ppid);
    return p->p_ppid;
}

pid_t wait() { //TODO: statut
    klogf(Log_verb, "syscall", "wait");
    proc_t *p = &state.st_proc[state.st_curr_pid];
    if (p->p_nchd > 0) {

        for (pid_t pid = 2; pid < NPROC; pid++) {
            proc_t* cp = state.st_proc + pid;
            if (cp->p_ppid == state.st_curr_pid
                    && cp->p_stat == ZOMB) {
                cp->p_stat = FREE;
                return pid;
            }
        }

        p->p_stat = WAIT;
        klogf(Log_verb, "syscall", "process %d wait %d", p->p_pid, p->p_nchd);
        schedule_proc(1);
        kAssert(false);
        return 0;
    } else {
        klogf(Log_verb, "syscall",
              "process %d has no child. dont wait", p->p_pid);
        return -1;
    }
}

pid_t waitpid(pid_t pid) {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    if (p->p_nchd > 0) {
        if (state.st_proc[pid].p_ppid == p->p_pid &&
            state.st_proc[pid].p_stat == SLEEP) {
            p->p_stat = WAIT;
            klogf(Log_verb, "syscall",
                  "process %d wait %d", p->p_pid, p->p_nchd);
            schedule_proc(1);
            kAssert(false);
            return 0;
        } else {
            klogf(Log_verb, "syscall",
                  "process %d is not %d's child", pid, p->p_pid);
            return -1;
        }
    } else {
        klogf(Log_verb, "syscall",
              "process %d has no child. dont wait", p->p_pid);
        return -1;
    }
}

pid_t fork() {
    proc_t *fp, *p = &state.st_proc[state.st_curr_pid];

    pid_t pid = find_new_pid(state.st_curr_pid);
    // we didn't find place for a new processus
    if (pid == state.st_curr_pid)
        return -1;

    fp          = &state.st_proc[pid];
    fp->p_pid   = pid;
    fp->p_ppid  = state.st_curr_pid;
    fp->p_stat  = RUN;
    fp->p_pri   = p->p_pri;
    fp->p_nchd  = 0;
    p->p_nchd++; // one more child

    // TODO clean
    uint64_t *a = (uint64_t*)&fp->p_reg;
    uint64_t *b = (uint64_t*)&p->p_reg;

    for (size_t i = 0; i < 18; i++)
        a[i] = b[i];

    fp->p_pml4 = kmem_alloc_page();
    kmem_fork_paging(fp->p_pml4);

    // copy file descriptors
    for (int i = 0; i < NFD; i++) {
        cid_t cid = p->p_fds[i];
        fp->p_fds[i] = cid;
        if (cid >= 0 && cid < NCHAN &&
            state.st_chann[cid].chann_mode != UNUSED) {
            // one more process is referencing the channel
            state.st_chann[cid].chann_acc++;
        }
    }

    fp->p_reg.rax = 0;

    kAssert(!push_ps(pid));

    klogf(Log_info, "syscall", "fork %d into %d", p->p_pid, fp->p_pid);

    return pid;
}

int open(const char *fname, enum chann_mode mode) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    cid_t cid;
    for (cid = 0; cid < NCHAN; cid++)
        if (state.st_chann[cid].chann_mode == UNUSED) break;

    if (cid == NCHAN) {
        klogf(Log_error, "sys", "no channel available");
        return -1;
    }

    chann_t *c   = &state.st_chann[cid];
    c->chann_acc  = 1;
    c->chann_mode = mode;

    c->chann_vfile = vfs_load(fname, 1);
    if (!c->chann_vfile) {
        klogf(Log_error, "sys",
              "process %d couldn't open %s", p->p_pid, fname);
        return -1;
    }

    c->chann_pos   = 0;

    for (int fd = 0; fd < NFD; fd++) {
        if (p->p_fds[fd] == -1) {
            p->p_fds[fd] = cid;
            klogf(Log_info, "syscall",
                  "process %d open %s on %d", p->p_pid, fname, fd);
            return fd;
        }
    }

    return -1;
}

int close(int filedes) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    // invalid file descriptor
    if (filedes < 0 || filedes > NFD)
        return -1;

    // file descriptor reference no channel
    if (p->p_fds[filedes] == -1)
        return -1;

    chann_t *c = &state.st_chann[p->p_fds[filedes]];
    if (c->chann_mode != UNUSED && --c->chann_acc == 0) {
        c->chann_mode = UNUSED;
    }

    return 0;
}

int dup(int fd) {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    int i;

    if (fd < 0 || fd > NFD || p->p_fds[fd] == -1)
        return -1;

    for (i = 0; i < NFD && p->p_fds[i] != -1; i++);

    if (i == NFD)
        // no file free descriptor
        return -1;

    p->p_fds[i] = p->p_fds[fd];
    return i;
}

int pipe(int fd[2]) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    if (!fd) return -1;

    int fdin, fdout;

    for (fdin = 0; fdin < NFD && p->p_fds[fdin] != -1; fdin++);
    if (fd[0] == NFD) return -1;

    for (fdout = fdin; fdout < NFD && p->p_fds[fdout] != -1; fdout++);
    if (fd[0] == NFD) return -1;

    cid_t in, out;
    for (in = 0; in < NCHAN && state.st_chann[in].chann_mode != UNUSED; in++);
    if (in == NCHAN) return -1;

    for (out = in; out < NCHAN && state.st_chann[out].chann_mode != UNUSED; out++);
    if (out == NCHAN) return -1;

    chann_t *cin = &state.st_chann[in],
        *cout = &state.st_chann[out];

    fd[fdin] = in;
    fd[fdout] = out;

    cin->chann_acc   = 1;
    cin->chann_mode  = STREAM_IN;
    cout->chann_acc  = 1;
    cout->chann_mode = STREAM_OUT;

    kAssert(false);// TODO : finish

    return 0;
}

int read(int fd, uint8_t *d, size_t len) {
    proc_t *p  = &state.st_proc[state.st_curr_pid];;

    if (!d || fd < 0 || fd > NFD || p->p_fds[fd] == -1)
        return -1;

    chann_t *chann = &state.st_chann[p->p_fds[fd]];
    klogf(Log_verb, "syscall", "process %d read on %d", p->p_pid, fd);

    vfile_t *vfile = chann->chann_vfile;

    switch (chann->chann_mode) {
    case READ:
    case RDWR:
        vfs_seek(vfile, chann->chann_pos);
        return vfs_read(vfile, d, len);

    case STREAM_IN:
        kAssert(false); // TODO
        return -1;
    default:
        return -1;
    }
}

int write(int fd, uint8_t *s, size_t len) {
    proc_t *p  = &state.st_proc[state.st_curr_pid];;

    if (!s || fd < 0 || fd > NFD || p->p_fds[fd] == -1)
        return -1;

    chann_t *chann = &state.st_chann[p->p_fds[fd]];
    vfile_t *vfile = chann->chann_vfile;
    klogf(Log_verb, "syscall", "process %d write on %d", p->p_pid, fd);

    size_t c = 0;
    switch (chann->chann_mode) {
    case WRITE:
    case RDWR:
        vfs_seek(vfile, chann->chann_pos);
        return vfs_write(vfile, s, len);

    case STREAM_OUT:
        // TODO : more efficient
        for (c = 0; c < len; c++)
            kprintf("%c", *s++);
        return c;

    default:
        p->p_reg.rax = -1;
        return -1;
    }
}

off_t lseek(int fd, off_t off) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    if (fd < 0 || fd > NFD) {
        klogf(Log_error, "syscall",
              "process %d lseek on invalid file descriptor %d", p->p_pid, fd);
        return -1;
    }

    klogf(Log_verb, "syscall",
          "process %d lseek %d at %d", p->p_pid, fd, off);

    state.st_chann[p->p_fds[fd]].chann_pos = off;

    return off;
}

int invalid_syscall() {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    klogf(Log_error, "syscall", "invalid syscall code %d", p->p_reg.rax);
    return -1;
}
