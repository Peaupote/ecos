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

void sys_exit(int status) {
    klogf(Log_info, "syscall", "exit pid %d with status %d",
          state.st_curr_pid, status);

    kill_proc_nr(WSTATUS_EXITED|(status&0xff));
}

pid_t sys_getpid() {
    klogf(Log_verb, "syscall", "getpid %d", state.st_curr_pid);
    set_errno(SUCC);
    return state.st_curr_pid;
}

pid_t sys_getppid() {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    klogf(Log_verb, "syscall", "getppid %d", p->p_ppid);
    set_errno(SUCC);
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
	if (rt_st && !check_argW(rt_st, sizeof(int))) {
		set_errno(EINVAL);
		return -1;
	}
    klogf(Log_verb, "syscall", "wait");
    proc_t *p = &state.st_proc[state.st_curr_pid];
    if (~p->p_fchd) {
        // Si il y a un zombie alors le premier enfant en est un
        proc_t* cp = state.st_proc + p->p_fchd;
        if (cp->p_stat == ZOMB) {

            pid_t cpid = p->p_fchd;
            if (rt_st)
                *rt_st = cp->p_reg.r.rdi.i;

            rem_child_Z0(p, cp);

            --p->p_nchd;
            free_pid(cpid);
            return cpid;

        } else {
            proc_self_block(p);
            klogf(Log_info, "syscall", "process %d wait %d childs",
                    state.st_curr_pid, p->p_nchd);
            schedule_proc();
        }

    } else {
        klogf(Log_verb, "syscall",
              "process %d has no child. dont wait", state.st_curr_pid);
        set_errno(ECHILD);
        return -1;
    }
}

pid_t sys_waitpid(pid_t cpid, int* rt_st) {
	if (rt_st && !check_argW(rt_st, sizeof(int))) {
		set_errno(EINVAL);
		return -1;
	}
	
	pid_t mpid = state.st_curr_pid;
    proc_t  *p = state.st_proc + mpid;

    if (cpid == -1) {
		cpid = p->p_fchd;
		proc_t* cp = state.st_proc + cpid;
		if (~cpid) {
			if (cp->p_stat == ZOMB) {
				// Il y a des ZOMB

				if (rt_st)
					*rt_st = cp->p_reg.r.rdi.i;

				rem_child_Z0(p, cp);

				--p->p_nchd;
				free_pid(cpid);
				return cpid;

			} else {
				// Il n'y a pas de ZOMB, on cherche des STOP
				for (; ~cpid; cpid = cp->p_nxsb) {
					cp = state.st_proc + cpid;
					if (cp->p_stat == STOP && cp->p_stps & 0x100) {
						cp->p_stps &= 0xff;
						if (rt_st) *rt_st = WSTATUS_STOPPED|cp->p_stps;
						return cpid;
					}
				}

				proc_self_block(p);
				schedule_proc();
			}
		} else {
			// Pas d'enfant
			set_errno(ECHILD);
			return -1;
		}
	}

    proc_t *cp = state.st_proc + cpid;
    if (cp->p_ppid == mpid) {

        if (cp->p_stat == ZOMB) {

            if (rt_st) *rt_st = cp->p_reg.r.rdi.i;

            if (~cp->p_prsb) {
                state.st_proc[cp->p_prsb].p_nxzb = cp->p_nxzb;
                if (~cp->p_nxzb)
                    state.st_proc[cp->p_nxzb].p_prsb = cp->p_prsb;
            } else
                rem_child_Z0(p, cp);

            --p->p_nchd;
            free_pid(cpid);
            return cpid;
    
		} else if (cp->p_stat == STOP && cp->p_stps & 0x100) {
			cp->p_stps &= 0xff;
			if (rt_st) *rt_st = WSTATUS_STOPPED|cp->p_stps;
			return cpid;
		}

        proc_self_block(p);
        klogf(Log_info, "syscall", "process %d wait child %d",
                mpid, cpid);
        schedule_proc();

    } else {
        klogf(Log_info, "syscall", "process %d is not %d's child",
                cpid, mpid);
        set_errno(ECHILD);
        return -1;
    }
}

pid_t sys_fork() {
    proc_t *fp, *pp = cur_proc();
	
    pid_t fpid = find_new_pid();
	
    // we didn't find place for a new processus
    if (!~fpid) {
        set_errno(EAGAIN);
        return -1;
    }

    fp        = state.st_proc + fpid;
    fp->p_ppid = state.st_curr_pid;

    // Ajout dans la liste des enfants
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

    fp->p_fchd   = PID_NONE;
    fp->p_nchd   = 0;
    fp->p_stat   = RUN;
    fp->p_ring   = pp->p_ring;
    fp->p_prio   = pp->p_prio;
    fp->p_brk    = pp->p_brk;
	fp->p_brkm   = pp->p_brkm;
    fp->p_spnd   = 0;
    fp->p_shnd   = pp->p_shnd;
    fp->p_errno  = pp->p_errno;
	fp->p_werrno = pp->p_werrno;
    fp->p_cino   = pp->p_cino;
    fp->p_dev    = pp->p_dev;

    pp->p_nchd++; // one more child
    memcpy(&fp->p_reg, &pp->p_reg, sizeof(struct reg));
	strncpy(fp->p_cmd, pp->p_cmd, 256);

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

    fp->p_reg.r.rax.pid_t = 0;

    sched_add_proc(fpid);

    klogf(Log_info, "syscall", "fork %d into %d",
            state.st_curr_pid, fpid);

    return fpid; // On retourne au parent
}

int sys_setpriority(int prio) {
    proc_t *p = state.st_proc + state.st_curr_pid;
	if (prio < 0 || prio >= NB_PRIORITY_LVL - 1) {
		set_errno(EINVAL);
		return -1;
	}
    if (prio > p->p_prio) {
		set_errno(EPERM);
		return -1;
	}
    p->p_prio = prio;
	set_errno(SUCC);
    return 0;
}
int sys_getpriority() {
    return state.st_proc[state.st_curr_pid].p_prio;
}

void* sys_sbrk(intptr_t inc) {
    proc_t *p = state.st_proc + state.st_curr_pid;
    uint_ptr nbrk = (uint_ptr)( ((intptr_t)p->p_brk) + inc );

	if (nbrk < p->p_brkm) {
		klogf(Log_warn, "sbrk", "trying to go below brkm");
		set_errno(ENOMEM);
		return (void*)-1;
	} else if (nbrk > p->p_brkm + PROC_MAX_BRK) {
		klogf(Log_warn, "sbrk", "too much memory asked");
		set_errno(ENOMEM);
		return (void*)-1;
	}

	if (nbrk > p->p_brk) {
		kmem_paging_alloc_rng(align_to(p->p_brk, PAGE_SIZE), nbrk,
				PAGING_FLAG_W | PAGING_FLAG_U,
				PAGING_FLAG_W | PAGING_FLAG_U);
	} else {
		uint_ptr rbg = align_to(nbrk, PAGE_SIZE);
		for (uint_ptr a = rbg; a < p->p_brk; a += PAGE_SIZE)
			kmem_free_paging_range(paging_page_entry(a), pgg_pt, 1);
	}
    void *rt = (void*) p->p_brk;
    p->p_brk = nbrk;
    set_errno(SUCC);
    return rt;
}

int sys_pause() {
    //On bloque le processus actuel
    proc_t* p = cur_proc();
    proc_self_block(p);
    schedule_proc();
}

uint64_t invalid_syscall() {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    klogf(Log_error, "syscall", "invalid syscall code %lld",
            p->p_reg.r.rax.ll);
    return ~(uint64_t)0;
}

// Appels système de privilège ring 1

void prires_proc_struct(uint16_t new_ring) {
    state.st_proc[state.st_curr_pid].p_ring = new_ring & 0xff;
}
