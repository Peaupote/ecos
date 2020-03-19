#include <stdint.h>

#include "../int.h"
#include "../proc.h"
#include "../kutil.h"
#include "../sys.h"
#include "../memory/shared_pages.h"

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
        pp->p_reg.rax = status;
    }

    pp->p_nchd--; // one child less

    p->p_ppid = 0;
    p->p_stat = FREE;
    for (pid_t pid = 2; pid < NPROC; pid++) {
        pp = &state.st_proc[pid];
        if (pp->p_ppid == state.st_curr_pid) {
            pp->p_ppid = 1;
            state.st_proc[1].p_nchd++;
        }
    }

    schedule_proc(1);
}

void getpid() {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    p->p_reg.rax = p->p_pid;
    klogf(Log_info, "syscall", "getpid %d", p->p_pid);
}

void getppid() {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    p->p_reg.rax = p->p_ppid;
    klogf(Log_info, "syscall", "getppid %d", p->p_ppid);
}

void wait() {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    if (p->p_nchd > 0) {
        p->p_stat = WAIT;
        klogf(Log_info, "syscall", "process %d wait %d", p->p_pid, p->p_nchd);
        schedule_proc(1);
    } else {
        klogf(Log_info, "syscall",
              "process %d has no child. dont wait", p->p_pid);
    }
}

void waitpid() {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    pid_t pid = p->p_reg.rdi;
    if (p->p_nchd > 0) {
        if (state.st_proc[pid].p_ppid == p->p_pid &&
            state.st_proc[pid].p_stat == SLEEP) {
            p->p_stat = WAIT;
            klogf(Log_info, "syscall",
                  "process %d wait %d", p->p_pid, p->p_nchd);
            schedule_proc(1);
        } else {
            klogf(Log_info, "syscall",
                  "process %d is not %d's child", pid, p->p_pid);
        }
    } else {
        klogf(Log_info, "syscall",
              "process %d has no child. dont wait", p->p_pid);
    }
}

void fork() {
    proc_t *fp, *p = &state.st_proc[state.st_curr_pid];

    // TODO : find more efficient way to choose new pid
    pid_t pid;
    for (pid = state.st_curr_pid; pid < NPROC; pid++)
        if (state.st_proc[pid].p_stat == FREE) break;

    if (pid == NPROC) {
        for (pid = 2; pid < state.st_curr_pid; pid++)
            if (state.st_proc[pid].p_stat == FREE) break;
    }

    // we didn't find place for a new processus
    if (pid == state.st_curr_pid) {
        p->p_reg.rax = -1;
        return;
    }

    fp         = &state.st_proc[pid];
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
	pml4_to_cr3(fp->p_pml4);

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

    p->p_reg.rax  = pid;
    fp->p_reg.rax = 0;

    switch_proc(fp->p_pid);
    klogf(Log_info, "syscall", "fork %d into %d", p->p_pid, fp->p_pid);
}

void open() {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    char *fname = (char*)p->p_reg.rdi;
    enum chann_mode mode = p->p_reg.rsi;

    // TODO : find file on disc
    // TODO : more efficient
    cid_t cid;
    for (cid = 0; cid < NCHAN; cid++)
        if (state.st_chann[cid].chann_mode == UNUSED) break;

    if (cid == NCHAN) {
        p->p_reg.rax = -1;
        return;
    }

    chann_t *c   = &state.st_chann[cid];
    c->chann_acc  = 1;
    c->chann_mode = mode;
    c->chann_buf  = load_buffer(1, 0);
    c->chann_pos  = 0;

    for (int fd = 0; fd < NFD; fd++)
        if (p->p_fds[fd] == -1) {
            p->p_fds[fd] = cid;
            p->p_reg.rax = fd;
            klogf(Log_info, "syscall",
                  "process %d open %s on %d", p->p_pid, fname, fd);
            return;
        }

    p->p_reg.rax = -1;
}

void close() {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    int filedes = p->p_reg.rdi;

    // invalid file descriptor
    if (filedes < 0 || filedes > NFD) {
        p->p_reg.rax = -1;
        return;
    }

    // file descriptor reference no channel
    if (p->p_fds[filedes] == -1) {
        p->p_reg.rax = -1;
        return;
    }

    chann_t *c = &state.st_chann[p->p_fds[filedes]];
    if (c->chann_mode != UNUSED && --c->chann_acc == 0) {
        c->chann_mode = UNUSED;
    }

    p->p_reg.rax = 0;
}

void dup() {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    int i, fd = p->p_reg.rdi;

    if (fd < 0 || fd > NFD || p->p_fds[fd] == -1) {
        p->p_reg.rax = -1;
        return;
    }

    for (i = 0; i < NFD && p->p_fds[i] != -1; i++);

    if (i == NFD) {
        // no file free descriptor
        p->p_reg.rax = -1;
        return;
    }

    p->p_fds[i] = p->p_fds[fd];
}

void pipe() {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    int *fd   = (int*)p->p_reg.rdi;

    if (!fd) goto error;

    int fdin, fdout;

    for (fdin = 0; fdin < NFD && p->p_fds[fdin] != -1; fdin++);
    if (fd[0] == NFD) goto error;

    for (fdout = fdin; fdout < NFD && p->p_fds[fdout] != -1; fdout++);
    if (fd[0] == NFD) goto error;

    cid_t in, out;
    for (in = 0; in < NCHAN && state.st_chann[in].chann_mode != UNUSED; in++);
    if (in == NCHAN) goto error;

    for (out = in; out < NCHAN && state.st_chann[out].chann_mode != UNUSED; out++);
    if (out == NCHAN) goto error;

    chann_t *cin = &state.st_chann[in],
        *cout = &state.st_chann[out];

    fd[fdin] = in;
    fd[fdout] = out;

    cin->chann_acc   = 1;
    cin->chann_mode  = STREAM_IN;
    cout->chann_acc  = 1;
    cout->chann_mode = STREAM_OUT;

    // TODO : finish

    return;
error:
    p->p_reg.rax = -1;
    return;
}

void read() {
    proc_t *p  = &state.st_proc[state.st_curr_pid];;
    int fd     = p->p_reg.rdi;
    uint8_t *v = (uint8_t*)p->p_reg.rsi;
    size_t len = p->p_reg.rdx;

    if (!v || fd < 0 || fd > NFD || p->p_fds[fd] == -1) {
        p->p_reg.rax = -1;
        return;
    }

    chann_t *chann = &state.st_chann[p->p_fds[fd]];
    klogf(Log_info, "syscall", "process %d read on %d", p->p_pid, fd);

    buf_t *buf = chann->chann_buf;
    size_t i;

    switch (chann->chann_mode) {
    case READ:
    case RDWR:
        for (i = 0; i < len; i++) {
            if (chann->chann_pos >= buf->buf_size) {
                if (buf->buf_nx) {
                    chann->chann_buf = buf->buf_nx;
                    chann->chann_pos = 0;
                    buf = buf->buf_nx;
                } else break;
            }

            v[i] = buf->buf_content[chann->chann_pos++];
        }

        p->p_reg.rax = i; // return number of read chars
        break;

    case STREAM_IN:
        // TODO
    default:
        p->p_reg.rax = -1;
        return;
    }
}

void write() {
    proc_t *p  = &state.st_proc[state.st_curr_pid];;
    int fd     = p->p_reg.rdi;
    uint8_t *v = (uint8_t*)p->p_reg.rsi;
    size_t len = p->p_reg.rdx;

    if (!v || fd < 0 || fd > NFD || p->p_fds[fd] == -1) {
        p->p_reg.rax = -1;
        return;
    }

    chann_t *chann = &state.st_chann[p->p_fds[fd]];
    buf_t   *buf   = chann->chann_buf;
    klogf(Log_info, "syscall", "process %d write on %d", p->p_pid, fd);

    size_t c = 0;
    switch (chann->chann_mode) {
    case WRITE:
    case RDWR:
        for (c = 0; c < len; c++) {
            if (chann->chann_pos >= buf->buf_size) {
                if (buf->buf_nx) {
                    chann->chann_buf = buf->buf_nx;
                    chann->chann_pos = 0;
                    buf = buf->buf_nx;
                } else break;
            }

            buf->buf_content[chann->chann_pos++] = *v++;
        }

        p->p_reg.rax = c;
        break;

    case STREAM_OUT:
        // TODO : more efficient
        for (c = 0; c < len; c++)
            kprintf("%c", *v++);
        p->p_reg.rax = c;
        break;

    default:
        p->p_reg.rax = -1;
        break;
    }
}

void lseek(void) {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    int fd = p->p_reg.rdi;
    pos_t off = p->p_reg.rsi;

    // TODO : checks
    klogf(Log_info, "syscall",
          "process %d lseek %d at %d", p->p_pid, fd, off);

    // TODO : complete
    state.st_chann[p->p_fds[fd]].chann_pos = off;
}

void invalid_syscall() {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    klogf(Log_error, "syscall", "invalid syscall code %d", p->p_reg.rax);
}
