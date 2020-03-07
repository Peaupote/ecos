#include <stdint.h>

#include "../int.h"
#include "../proc.h"
#include "../kutil.h"
#include "../sys.h"

/**
 * Syscalls
 */

void kexit() {
    proc_t *p = &state.st_proc[state.st_curr_pid],
        *pp = &state.st_proc[p->p_ppid];

    int status = p->p_reg.rdi;
    klogf(Log_info, "syscall",
          "kill pid %d with status %d", p->p_pid, status);

    if (pp->p_stat == WAIT) {
        pp->p_stat    = RUN;
        pp->p_reg.rax = status;
    }

    pp->p_nchd--; // one child less

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
    // TODO
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
    kmem_copy_paging(fp->p_pml4);

    // copy file descriptors
    for (int i = 0; i < NFD; i++)
        fp->p_fds[i] = p->p_fds[i];

    p->p_reg.rax  = pid;
    fp->p_reg.rax = 0;

    switch_proc(fp->p_pid);
    klogf(Log_info, "syscall", "fork %d into %d", p->p_pid, fp->p_pid);
}

int open() {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    char *fname = (char*)p->p_reg.rdi;
    enum chann_mode mode = p->p_reg.rsi;

    // TODO : find file on disc

    // TODO : more efficient
    int cid;
    for (cid = 0; cid < NCHAN; cid++)
        if (state.st_chann[cid].chann_mode == UNUSED) break;

    chann_t *c = &state.st_chann[cid];
    c->chann_acc  = 1;
    c->chann_mode = mode;
    c->chann_file = 0;

    for (int fd = 0; fd < NFD; fd++)
        if (p->p_fds[fd] == -1) {
            p->p_fds[fd] = cid;
            return fd;
        }

    return -1;
}

int close() {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    int filedes = p->p_reg.rdi;

    // file descriptor reference no channel
    if (p->p_fds[filedes] == -1) return -1;
    chann_t *c = &state.st_chann[p->p_fds[filedes]];

    if (--c->chann_acc == 0) { c->chann_mode = UNUSED; }
    return 0;
}


void invalid_syscall() {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    klogf(Log_error, "syscall", "invalid syscall code %d", p->p_reg.rax);
}
