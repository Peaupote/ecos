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

/**
 * Syscalls
 * TODO: check args domain
 */

void sys_exit(int status) {
    proc_t  *p = state.st_proc + state.st_curr_pid;
    pid_t ppid = p->p_ppid;
    proc_t *pp = state.st_proc + ppid;

    klogf(Log_info, "syscall", "kill pid %d with status %d ppid=%d(%c)",
          state.st_curr_pid, status, ppid,
          proc_state_char[pp->p_stat]);

    if (~p->p_fchd) {
        // Les enfants du processus reçoivent INIT comme nouveau parent
        proc_t* fcp = state.st_proc + p->p_fchd;
        proc_t*  ip = state.st_proc + PID_INIT;

        if (fcp->p_stat == ZOMB) {

            pid_t zcpid = p->p_fchd;
            proc_t* zcp = fcp;
            goto loop_enter_z;
            while (~zcp->p_nxzb) {// Zombies
                zcpid = zcp->p_nxzb;
                zcp   = state.st_proc + zcpid;
            loop_enter_z:
                zcp->p_ppid = PID_INIT;
            }
            pid_t cpid = p->p_fchd;
            proc_t* cp = fcp;
            while (~cp->p_nxsb) {
                cpid = cp->p_nxsb;
                cp   = state.st_proc + cpid;
                cp->p_ppid = PID_INIT;
            }

            if (~ip->p_fchd) {
                proc_t* ifcp = state.st_proc + ip->p_fchd;
                if (ifcp->p_stat == ZOMB) {
                    cp ->p_nxsb = ifcp->p_nxsb;
                    if (~ifcp->p_nxsb)
                        state.st_proc[ifcp->p_nxsb].p_prsb = cpid;
                    zcp->p_nxzb = ifcp->p_nxzb;
                    if (~ifcp->p_nxzb)
                        state.st_proc[ifcp->p_nxzb].p_prsb = zcpid;
                } else {
                    cp->p_nxsb   = ip->p_fchd;
                    ifcp->p_prsb = cpid;
                }
            }
            ip->p_fchd = p->p_fchd;

        } else {

            pid_t cpid = p->p_fchd;
            proc_t* cp = fcp;
            goto loop_enter_l;
            while (~cp->p_nxsb) {
                cpid = cp->p_nxsb;
                cp   = state.st_proc + cpid;
            loop_enter_l:
                cp->p_ppid = PID_INIT;
            }

            if (~ip->p_fchd) {
                proc_t* ifcp = state.st_proc + ip->p_fchd;
                if (ifcp->p_stat == ZOMB) {
                    cp -> p_nxsb = ifcp->p_nxsb;
                    if (~ifcp->p_nxsb)
                        state.st_proc[ifcp->p_nxsb].p_prsb = cpid;
                    ifcp->p_nxsb = p->p_fchd;
                    fcp ->p_prsb = ip->p_fchd;
                } else {
                    cp->p_nxsb   = ip->p_fchd;
                    ifcp->p_prsb = cpid;
                    ip->p_fchd   = p->p_fchd;
                }
            } else
                ip->p_fchd = p->p_fchd;
        }
        ip->p_nchd += p->p_nchd;
    }

    if (~p->p_nxsb)
        state.st_proc[p->p_nxsb].p_prsb = p->p_prsb;

    if (~p->p_prsb)
        state.st_proc[p->p_prsb].p_nxsb = p->p_nxsb;
    else
        pp->p_fchd = p->p_nxsb;

    if (pp->p_stat == WAIT
        && (rei_cast(pid_t, pp->p_reg.rax) == PID_NONE
            || rei_cast(pid_t, pp->p_reg.rax) == state.st_curr_pid)) {

        kmem_free_paging(p->p_pml4,
                pp->p_pml4 ? pp->p_pml4 : kernel_pml4);

        free_pid(state.st_curr_pid);
        pp->p_nchd--;
        pp->p_reg.rax = state.st_curr_pid;
        pp->p_stat    = RUN;

        proc_set_curr_pid(ppid);
        int* rt_st = rei_cast(int*, pp->p_reg.rsi);
        if (rt_st)
            *rt_st = status;

        iret_to_proc(pp);
    } else {
        // on réajoute le processus comme premier enfant
        p->p_prsb = PID_NONE;
        if (~pp->p_fchd) {
            proc_t* fc = state.st_proc + pp->p_fchd;
            fc->p_prsb = state.st_curr_pid;
            if (fc->p_stat == ZOMB) {
                if (~fc->p_nxsb)
                    state.st_proc[fc->p_nxsb].p_prsb = state.st_curr_pid;
                p->p_nxsb = fc->p_nxsb;
                p->p_nxzb = pp->p_fchd;
            } else {
                p->p_nxzb = PID_NONE;
                p->p_nxsb = pp->p_fchd;
            }
        } else
            p->p_nxsb = p->p_nxzb = PID_NONE;
        pp->p_fchd = state.st_curr_pid;

        kmem_free_paging(p->p_pml4, kernel_pml4);
        p->p_stat = ZOMB;// statut dans rdi
        schedule_proc();
    }
    never_reached
}

pid_t sys_getpid() {
    klogf(Log_verb, "syscall", "getpid %d", state.st_curr_pid);
    return state.st_curr_pid;
}

pid_t sys_getppid() {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    klogf(Log_verb, "syscall", "getppid %d", p->p_ppid);
    return p->p_ppid;
}

static inline void rem_child_Z0(proc_t* p, proc_t* cp) {
    if (~cp->p_nxzb) {
        p->p_fchd = cp->p_nxzb;
        state.st_proc[cp->p_nxzb].p_prsb = PID_NONE;
        state.st_proc[cp->p_nxzb].p_nxsb = p->p_nxsb;
        if (~cp->p_nxsb)
            state.st_proc[cp->p_nxsb].p_prsb = cp->p_nxzb;
    } else {
        p->p_fchd = cp->p_nxsb;
        if (~cp->p_nxsb)
            state.st_proc[cp->p_nxsb].p_prsb = PID_NONE;
    }
}

pid_t sys_wait(int* rt_st) {
    klogf(Log_verb, "syscall", "wait");
    proc_t *p = &state.st_proc[state.st_curr_pid];
    if (~p->p_fchd) {
        // Si il y a un zombie alors le premier enfant en est un
        proc_t* cp = state.st_proc + p->p_fchd;
        if (cp->p_stat == ZOMB) {

            pid_t cpid = p->p_fchd;
            if (rt_st)
                *rt_st = cp->p_reg.rdi;

            rem_child_Z0(p, cp);

            --p->p_nchd;
            free_pid(cpid);
            return cpid;

        } else {
            p->p_stat = WAIT;
            rei_cast(pid_t, p->p_reg.rax) = PID_NONE;
            klogf(Log_info, "syscall", "process %d wait %d childs",
                    state.st_curr_pid, p->p_nchd);
            schedule_proc();
            never_reached return 0;
        }

    } else {
        klogf(Log_verb, "syscall",
              "process %d has no child. dont wait", state.st_curr_pid);
        return -1;
    }
}

pid_t sys_waitpid(int* rt_st, pid_t cpid) {
    if (cpid == PID_NONE) return sys_wait(rt_st);

    pid_t mpid = state.st_curr_pid;
    proc_t  *p = state.st_proc + mpid;
    proc_t *cp = state.st_proc + cpid;
    if (cp->p_ppid == mpid) {

        if (cp->p_stat == ZOMB) {

            if (rt_st) *rt_st = cp->p_reg.rdi;

            if (~cp->p_prsb) {
                state.st_proc[cp->p_prsb].p_nxzb = cp->p_nxzb;
                if (~cp->p_nxzb)
                    state.st_proc[cp->p_nxzb].p_prsb = cp->p_prsb;
            } else
                rem_child_Z0(p, cp);

            --p->p_nchd;
            free_pid(cpid);
            return cpid;
        }

        p->p_stat = WAIT;
        rei_cast(pid_t, p->p_reg.rax) = cpid;
        klogf(Log_info, "syscall", "process %d wait child %d", mpid, cpid);
        schedule_proc();
        never_reached return 0;

    } else {
        klogf(Log_info, "syscall", "process %d is not %d's child",
                cpid, mpid);
        return -1;
    }
}

pid_t sys_fork() {
    proc_t *fp, *pp = state.st_proc + state.st_curr_pid;

    pid_t fpid = find_new_pid();
    // we didn't find place for a new processus
    if (!~fpid) return -1;

    fp          = state.st_proc + fpid;
    fp->p_ppid  = state.st_curr_pid;

    if (~pp->p_fchd) {
        proc_t *fc = state.st_proc + pp->p_fchd;
        if (fc->p_stat == ZOMB) {
            fp->p_prsb = pp->p_fchd;
            fp->p_nxsb = fc->p_nxsb;
            if (~fc->p_nxsb)
                state.st_proc[fc->p_nxsb].p_prsb = fpid;
            fc->p_nxsb = fpid;
        } else {
            fc->p_prsb = fpid;
            fp->p_nxsb = pp->p_fchd;
            fp->p_prsb = PID_NONE;
            pp->p_fchd = fpid;
        }
    } else {
        fp->p_nxsb = PID_NONE;
        fp->p_prsb = PID_NONE;
        pp->p_fchd = fpid;
    }

    fp->p_fchd  = PID_NONE;
    fp->p_nchd  = 0;
    fp->p_stat  = RUN;
    fp->p_ring  = pp->p_ring;
    fp->p_prio  = pp->p_prio;

    pp->p_nchd++; // one more child
    memcpy(&fp->p_reg, &pp->p_reg, sizeof(struct reg));

    fp->p_pml4 = kmem_alloc_page();
    kmem_fork_paging(fp->p_pml4);
    paging_refresh();

    // copy file descriptors
    for (int i = 0; i < NFD; i++) {
        cid_t cid = pp->p_fds[i];
        fp->p_fds[i] = cid;
        if (cid >= 0 && cid < NCHAN &&
            state.st_chann[cid].chann_mode != UNUSED) {
            // one more process is referencing the channel
            state.st_chann[cid].chann_acc++;
        }
    }

    proc_create(fpid);
    fp->p_reg.rax = 0;

    sched_add_proc(fpid);

    klogf(Log_info, "syscall", "fork %d into %d",
            state.st_curr_pid, fpid);

    return fpid; // On retourne au parent
}

int sys_open(const char *fname, enum chann_mode mode) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    cid_t cid;
    for (cid = 0; cid < NCHAN; cid++)
        if (state.st_chann[cid].chann_mode == UNUSED) break;

    if (cid == NCHAN) {
        klogf(Log_error, "sys", "no channel available");
        return -1;
    }

    chann_t *c = state.st_chann + cid;
    c->chann_vfile = vfs_load(fname, 1);
    if (!c->chann_vfile) {
        klogf(Log_error, "sys", "process %d couldn't open %s",
                state.st_curr_pid, fname);
        return -1;
    }

    for (int fd = 0; fd < NFD; fd++) {
        if (p->p_fds[fd] == -1) {
            c->chann_acc  = 1;
            c->chann_mode = mode;
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
    return i;
}

int sys_pipe(int fd[2]) {
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

int sys_read(int fd, uint8_t *d, size_t len) {
    proc_t *p  = &state.st_proc[state.st_curr_pid];;

    if (!d || fd < 0 || fd > NFD || p->p_fds[fd] == -1)
        return -1;

    chann_t *chann = &state.st_chann[p->p_fds[fd]];
    klogf(Log_info, "syscall", "process %d read %d on %d (cid %d)",
          state.st_curr_pid, len, fd, p->p_fds[fd]);

    vfile_t *vfile = chann->chann_vfile;

    int rc;
    switch (chann->chann_mode) {
    case READ:
    case RDWR:
        rc = vfs_read(vfile, d, chann->chann_pos, len);
        if (rc > 0) chann->chann_pos += rc;
        klogf(Log_verb, "syscall", "%d char readed (pos %d)",
              rc, chann->chann_pos);
        return rc;

    case STREAM_IN:
        return vfs_read(vfile, d, 0, len);
        break;

    default:
        return -1;
    }
}

int sys_write(int fd, uint8_t *s, size_t len) {
    proc_t *p  = &state.st_proc[state.st_curr_pid];;

    if (!s || fd < 0 || fd > NFD || p->p_fds[fd] == -1)
        return -1;

    chann_t *chann = &state.st_chann[p->p_fds[fd]];
    vfile_t *vfile = chann->chann_vfile;
    klogf(Log_verb, "syscall", "process %d write on %d",
            state.st_curr_pid, fd);

    size_t c = 0;
    int rc = 0;
    switch (chann->chann_mode) {
    case WRITE:
    case RDWR:
        rc = vfs_write(vfile, s, chann->chann_pos, len);
        if (rc > 0) chann->chann_pos += rc;
        return rc;

        //case STREAM_IN:
        //    return vfs_write(vfile, s, 0, len);

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

int sys_debug_block(int v) {
	if (~v) return v;

	//On bloque le processus actuel
	state.st_proc[state.st_curr_pid].p_stat = BLOCK;
	schedule_proc();
	never_reached return -1;
}

uint64_t invalid_syscall() {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    klogf(Log_error, "syscall", "invalid syscall code %d", p->p_reg.rax);
    return ~(uint64_t)0;
}

// Appels système de privilège ring 1

void prires_proc_struct(uint16_t new_ring) {
    state.st_proc[state.st_curr_pid].p_ring = rei_cast(uint8_t, new_ring);
}
