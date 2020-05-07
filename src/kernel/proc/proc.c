#include <kernel/proc.h>

#include <stddef.h>
#include <stdint.h>

#include <kernel/kutil.h>
#include <kernel/int.h>
#include <kernel/sys.h>
#include <kernel/int.h>
#include <kernel/tty.h>
#include <kernel/gdt.h>
#include <kernel/memory/kmem.h>
#include <kernel/file.h>

#include <libc/sys/wait.h>
#include <libc/stdio.h>
#include <libc/string.h>

#include <util/celf.h>
#include <util/misc.h>

#include <fs/proc.h>

char proc_state_char[7] = {'f', 'S', 'B', 'L', 'R', 'Z', 'T'};

extern void proc_idle_entry(void);
extern void proc_init_entry(void);

extern char libc_celf[];

void sched_init() {
    struct scheduler* s = &state.st_sched;
    s->pres    = 0;
    s->nb_proc = 0;
    for (size_t p = 0; p < NB_PRIORITY_LVL; ++p) {
        s->files[p].first = PID_NONE;
        s->files[p].last  = PID_NONE;
    }
}

static void libc_ldr_fill0(void* err_pt, 
		Elf64_Xword flag __attribute__((unused)),
        Elf64_Addr dst, uint64_t sz) {
	for (uint_ptr it = dst & PAGE_MASK; it < dst + sz; it += PAGE_SIZE) {
		uint64_t* e = kmem_acc_pts_entry(it, pgg_pt,
						PAGING_FLAG_W | PAGING_FLAG_U);
		if (!e) {
			*((uint8_t*) err_pt) = 1;
			return;
		}
		*e = SPAGING_ALLOC0 | PAGING_FLAG_W | PAGING_FLAG_U;
	}
}
static void libc_ldr_copy(void* err_pt, 
		Elf64_Xword flag __attribute__((unused)),
        Elf64_Addr dst, void* src, uint64_t sz) {
    uint8_t err = kmem_paging_alloc_rng(dst, dst + sz,
						PAGING_FLAG_U | PAGING_FLAG_W,
						PAGING_FLAG_U | PAGING_FLAG_W);
    if (err) {
        *((uint8_t*)err_pt) = err;
        return;
    }
	memcpy((void*)dst, src, sz);
	for (uint_ptr it = dst & PAGE_MASK; it < dst + sz; it += PAGE_SIZE) {
		uint64_t* e = paging_page_entry(it);
		clear_flag_64(e, PAGING_FLAG_W);
		*e |= SPAGING_FLAG_V; // Shared
	}
}

uint64_t libc_shared_idx;

static uint8_t load_libc() {
    struct elf_loader libc_ldr = {
        .fill0 = &libc_ldr_fill0,
        .copy  = &libc_ldr_copy
    };
    uint8_t err = 0;
    celf_load(libc_ldr, &err, libc_celf);
    if (err) return err;

	uint64_t* lc_e = paging_acc_pd(0, 0, PD_LIBC);
	struct sptr_hd* sp_hd;
	kmem_mk_svalue(lc_e, &libc_shared_idx, &sp_hd);
	sp_hd->count = 2;

	paging_refresh();

    return 0;
}

void proc_init() {
	kAssert(!load_libc());
    sched_init();

    // processus 0: p_idle
    proc_t *p_idle = &state.st_proc[PID_IDLE];
    p_idle->p_ppid = PID_IDLE;
    p_idle->p_prsb = p_idle->p_nxsb = PID_NONE;
    p_idle->p_fchd = PID_IDLE;
    p_idle->p_nchd = 1;
    p_idle->p_stat = RUN;
    p_idle->p_ring = 0;
    p_idle->p_prio = 0; //priorité minimale
    p_idle->p_pml4 = (phy_addr)NULL;
    p_idle->p_brkm = p_idle->p_brk = 0;
    p_idle->p_reg.rsp.p = kernel_stack_top;
    p_idle->p_reg.rip.p = &proc_idle_entry;
    strcpy(p_idle->p_cmd, "idle");
    p_idle->p_spnd = 0;
    p_idle->p_shnd.blk = 0;
    p_idle->p_shnd.ign = ~(sigset_t)0;
    p_idle->p_shnd.dfl = 0;

    // processus 1: init
    proc_t *p_init = &state.st_proc[PID_INIT];
    p_init->p_ppid = PID_INIT;
    p_init->p_prsb = p_init->p_nxsb = PID_NONE;
    p_init->p_fchd = PID_INIT;
    p_init->p_nchd = 1;
    p_init->p_stat = RUN;
    p_init->p_ring = 1;
    p_init->p_prio = NB_PRIORITY_LVL - 2; //priorité maximale
    p_init->p_pml4 = (phy_addr)NULL;
    p_init->p_brkm = p_init->p_brk = 0x1000;
    p_init->p_reg.rsp.p = NULL;
    p_init->p_reg.rip.p = &proc_init_entry;
    strcpy(p_init->p_cmd, "init");
    p_init->p_spnd = 0;
    p_init->p_shnd.blk = ~(sigset_t)0;
    p_init->p_shnd.ign = 0;
    p_init->p_shnd.dfl = ~(sigset_t)0;

    // processus 2: stop, utilisé en cas de panic
    proc_t *p_stop = &state.st_proc[PID_STOP];
    p_stop->p_ppid = PID_STOP;
    p_stop->p_nxwl = PID_NONE;
    p_stop->p_prwl = &p_stop->p_nxwl;
    p_stop->p_prsb = p_stop->p_nxsb = PID_NONE;
    p_stop->p_fchd = PID_STOP;
    p_stop->p_nchd = 1;
    p_stop->p_stat = BLOCK;
    p_stop->p_ring = 0;
    p_stop->p_prio = NB_PRIORITY_LVL - 1;
    p_stop->p_pml4 = (phy_addr)NULL;
    p_stop->p_brkm = p_stop->p_brk = 0;
    p_stop->p_reg.rsp.p = kernel_stack_top;
    p_stop->p_reg.rip.p = &proc_idle_entry;
    strcpy(p_stop->p_cmd, "stop");
    p_idle->p_spnd = 0;
    p_idle->p_shnd.blk = 0;
    p_idle->p_shnd.ign = ~(sigset_t)0;
    p_idle->p_shnd.dfl = 0;

    // set unused file desc
    for (size_t i = 0; i < NFD; i++)
        p_idle->p_fds[i] = p_stop->p_fds[i] = -1;
    for (size_t i = 3; i < NFD; i++) p_init->p_fds[i] = -1;

    // set chann to free
    for (cid_t cid = 0; cid < NCHAN; cid++) {
        state.st_chann[cid].chann_id      = cid;
        state.st_chann[cid].chann_mode    = UNUSED;
        state.st_chann[cid].chann_waiting = PID_NONE;
        state.st_chann[cid].chann_nxw     = cid;
    }

	for (pid_t pid = 0; pid < 3; ++pid) {
		// set work directory to root
		state.st_proc[pid].p_dev  = ROOT_DEV;
		state.st_proc[pid].p_cino = devices[ROOT_DEV].dev_info.root_ino;
		
		state.st_proc[pid].p_errno  = NULL;
		state.st_proc[pid].p_werrno = 0;
	}

    // set all remaining slots to free processus
    state.st_free_proc = 3;
    for (pid_t pid = 3; pid < NPROC; pid++) {
        state.st_proc[pid].p_stat = FREE;
        state.st_proc[pid].p_nxfr = pid + 1;
    }
    state.st_proc[NPROC - 1].p_nxfr = PID_NONE;
    state.st_free_proc_last = NPROC - 1;

	// no process owns ressources
	for (enum proc_owng o = 0; o < NB_OWNG; ++o)
		state.st_owng[o] = PID_NONE;

    sched_add_proc(PID_IDLE);

    state.st_curr_pid   = PID_INIT;
    st_curr_reg         = &p_init->p_reg;
	state.st_time_slice = SCHED_TIME_SLICE;

    kAssert(fs_proc_std_to_tty(p_init));
}

void sched_from_idle() {
    proc_t *p_idle = &state.st_proc[PID_IDLE];
    p_idle->p_reg.rsp.p = kernel_stack_top;
    p_idle->p_reg.rip.p = &proc_idle_entry;
    sched_add_proc(PID_IDLE);
	schedule_proc();
}

void proc_start(void) {
    proc_t* p_init = state.st_proc + PID_INIT;
    klogf(Log_info, "init", "Start process init @ %p", p_init->p_reg.rip.p);
    iret_to_proc(p_init);
}

void sched_add_proc(pid_t pid) {
    proc_t     *p = state.st_proc + pid;
    priority_t pr = p->p_prio;
    struct scheduler_file* pf = state.st_sched.files + pr;
    pid_t     prl = pf->last;

    klogf(Log_vverb, "sched", "add %d @ %d", (int)pid, (int)pr);

    ++state.st_sched.nb_proc;
    p->p_nxpf = PID_NONE;
    if (~prl) {
        state.st_proc[prl].p_nxpf = pid;
        p->p_prpf = &state.st_proc[prl].p_nxpf;
    } else {
        pf->first = pid;
        p->p_prpf = &pf->first;
        set_bit_64(&state.st_sched.pres, pr);
        state.st_sched.pres |= ((uint64_t)1) << pr;
    }
    pf->last = pid;
}

pid_t sched_pop_proc() {
    struct scheduler* s = &state.st_sched;
    priority_t pr = find_bit_64(s->pres, 1, 6);
    pid_t     pid = s->files[pr].first;
    pid_t      nx = state.st_proc[pid].p_nxpf;
    klogf(Log_vverb, "sched", "rem %d @ %d", (int)pid, (int)pr);

#ifdef __is_debug
    state.st_proc[pid].p_prpf = NULL;
#endif

    --s->nb_proc;
    s->files[pr].first = nx;
    if (~nx)
        state.st_proc[nx].p_prpf = &s->files[pr].first;
    else {
        s->files[pr].last = PID_NONE;
        clear_bit_64(&s->pres, pr);
    }

    return pid;
}

void _schedule_proc() {
    if (state.st_sched.pres) {
        kAssert(state.st_sched.nb_proc > 0);
        // pick a new process to run
        pid_t pid = sched_pop_proc();
        proc_t *p = switch_proc(pid);

        klogf(Log_info, "sched",
              "nb R %d, run proc %d :\n"
              "   rip %p, rsp %p",
              state.st_sched.nb_proc + 1, pid,
              p->p_reg.rip.p,
              p->p_reg.rsp.p);

		state.st_time_slice = SCHED_TIME_SLICE;
		if (p->p_werrno) {
			set_errno(p->p_werrno);
			p->p_werrno = 0;
		}
        proc_hndl_sigs();
        run_proc(p);
    }

    // Le processus IDLE empêche que l'on arrive ici
    never_reached
}

pid_t schedule_proc_ev() {
    sched_add_proc(state.st_curr_pid);
    pid_t rt = sched_pop_proc();
	state.st_time_slice = SCHED_TIME_SLICE;
    klogf(Log_vverb, "sched",
          "nb waiting %d choose proc %d",
          state.st_sched.nb_proc + 1,
          state.st_curr_pid);
    return rt;
}

proc_t *switch_proc(pid_t pid) {
    proc_t *p   = state.st_proc + pid;
    state.st_curr_pid = pid;
    st_curr_reg = &p->p_reg;

    if (p->p_pml4) pml4_to_cr3(p->p_pml4);

    return p;
}

void proc_ps() {
    for (pid_t pid = 0; pid < NPROC; pid++) {
        proc_t* p = state.st_proc + pid;
        if (p->p_stat != FREE)
            kprintf("%cpid=%d st=%c ppid=%d pr=%d pml4=%p '%s'\n",
                    pid==state.st_curr_pid ? '*' : ' ',
                    (int)pid, (int)proc_state_char[p->p_stat],
                    (int)p->p_ppid, 19 - (int)p->p_prio,
                    p->p_pml4, p->p_cmd);
    }
}

void wait_file(pid_t pid, cid_t cid) {
    klogf(Log_info, "proc", "pid %d waiting for channel %d", pid, cid);
    proc_t *p   = state.st_proc + pid;
    chann_t *c  = state.st_chann + cid;
    vfile_t *vf = c->chann_vfile;

    p->p_stat = BLOCK;
    p->p_nxwl = c->chann_waiting;
    if (~c->chann_waiting)
        state.st_proc[c->chann_waiting].p_prwl = &p->p_nxwl;
    c->chann_waiting = pid;
    p->p_prwl = &c->chann_waiting;

    if (c->chann_nxw == cid) {
        c->chann_nxw   = vf->vf_waiting;
        vf->vf_waiting = cid;
    }

    schedule_proc();
}

// --kill proc--

static void kill_proc_disown_rsr(pid_t pid) {
	if (state.st_owng[own_tty] == pid)
		tty_set_owner(PID_NONE);
}

static inline bool is_waiting_me(proc_t* pp, pid_t mpid) {
    return pp->p_stat == BLOCK && (
                pp->p_reg.r.rax.ll == SYS_WAIT
            || (pp->p_reg.r.rax.ll == SYS_WAITPID
                && pp->p_reg.r.rdi.pid_t == mpid)
            );
}

static void kill_proc_remove(pid_t pid, proc_t* p, proc_t* pp) {
	kill_proc_disown_rsr(pid);

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

		proc_unblock_1(PID_INIT, state.st_proc + PID_INIT);
    }

    if (~p->p_nxsb)
        state.st_proc[p->p_nxsb].p_prsb = p->p_prsb;

    if (~p->p_prsb) state.st_proc[p->p_prsb].p_nxsb = p->p_nxsb;
    else pp->p_fchd = p->p_nxsb;

    for (int fd = 0; fd < NFD; fd++) sys_close(fd);
}

static void kill_proc_2zb(proc_t* p, proc_t* pp, int status) {
    // on déplace le processus comme premier enfant
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
    p->p_stat        = ZOMB;
    p->p_reg.r.rdi.i = status;
}

void kill_proc_nr(int status) {
    proc_t  *p = state.st_proc + state.st_curr_pid;
    pid_t ppid = p->p_ppid;
    proc_t *pp = state.st_proc + ppid;

    kill_proc_remove(state.st_curr_pid, p, pp);

    if (is_waiting_me(pp, state.st_curr_pid)) {

        int* rt_st = pp->p_reg.r.rax.ll == SYS_WAIT
                   ? pp->p_reg.r.rdi.p
                   : pp->p_reg.r.rsi.p;

        kmem_free_paging(p->p_pml4,
                pp->p_pml4 ? pp->p_pml4 : kernel_pml4);

        free_pid(state.st_curr_pid);
        pp->p_nchd--;
        pp->p_reg.r.rax.pid_t = state.st_curr_pid;
        pp->p_stat            = RUN;

        proc_set_curr_pid(ppid);

        if (rt_st)
            *rt_st = status;

        iret_to_proc(pp);

    } else {
        kill_proc_2zb(p, pp, status);
        schedule_proc();
    }
}

void kill_proc(int status) {
    proc_t  *p = state.st_proc + state.st_curr_pid;
    pid_t ppid = p->p_ppid;
    proc_t *pp = state.st_proc + ppid;

    kill_proc_remove(state.st_curr_pid, p, pp);
    if (is_waiting_me(pp, state.st_curr_pid))
        proc_blocr(ppid, pp);
    kill_proc_2zb(p, pp, status);
}
