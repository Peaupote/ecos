#include <stdint.h>

#include "../proc.h"

/**
 * Syscalls
 */

void kexit(int status) {
    proc_t *p = &state.st_proc[state.st_curr_pid],
        *pp = &state.st_proc[p->p_ppid];

    if (pp->p_stat == WAIT) {
        pp->p_stat = RUN;
    }

    p->p_stat = FREE;
    for (pid_t pid = 2; pid < NPROC; pid++) {
        pp = &state.st_proc[pid];
        if (pp->p_ppid == state.st_curr_pid) {
            pp->p_ppid = 1;

            // not sure here
            pp->p_reg.rax = status;
            if (pp->p_stat != RUN) pp->p_stat = ZOMB;
        }
    }
}

pid_t wait() {
    // TODO
    return 0;
}

pid_t fork() {
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
    if (pid == state.st_curr_pid)
        return -1;

    fp          = &state.st_proc[pid];
    fp->p_pid   = pid;
    fp->p_ppid  = state.st_curr_pid;
    fp->p_stat  = RUN;
    fp->p_pri   = p->p_pri;
    fp->p_entry = p->p_entry;
    fp->p_rsp   = p->p_rsp;
    fp->p_pml4  = kmem_alloc_page();

    kmem_copy_paging(fp->p_pml4);

    // copy file descriptors
    for (int i = 0; i < NFD; i++)
        fp->p_fds[i] = p->p_fds[i];

    push_ps(p->p_pid);
    push_ps(pid);
    return pid;
}

int open(char *name, enum chann_mode mode) {
    // TODO : find file on disc

    // TODO : more efficient
    int cid;
    for (cid = 0; cid < NCHAN; cid++)
        if (state.st_chann[cid].chann_mode == UNUSED) break;

    chann_t *c = &state.st_chann[cid];
    c->chann_acc  = 1;
    c->chann_mode = mode;
    c->chann_file = 0;

    proc_t *p = &state.st_proc[state.st_curr_pid];
    for (int fd = 0; fd < NFD; fd++)
        if (p->p_fds[fd] == -1) {
            p->p_fds[fd] = cid;
            return fd;
        }

    return -1;
}

int close(int filedes) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    // file descriptor reference no channel
    if (p->p_fds[filedes] == -1) return -1;
    chann_t *c = &state.st_chann[p->p_fds[filedes]];

    if (--c->chann_acc == 0) { c->chann_mode = UNUSED; }
    return 0;
}
