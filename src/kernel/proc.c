#include <stddef.h>
#include <stdint.h>

#include <kernel/int.h>
#include <kernel/sys.h>
#include <kernel/proc.h>

#include <util/elf64.h>
#include <kernel/memory/kmem.h>
#include <kernel/memory/shared_pages.h>
#include <kernel/int.h>
#include <kernel/tty.h>
#include <kernel/kutil.h>
#include <kernel/gdt.h>
#include <util/misc.h>

char proc_state_char[5] = {'f', 's', 'w', 'R', 'Z'};

extern void    proc_idle_entry(void);
extern uint8_t proc_init[];

void sched_init() {
	struct scheduler* s = &state.st_sched;
	s->pres    = 0;
	s->nb_proc = 0;
	for (size_t p = 0; p < NB_PRIORITY_LVL; ++p) {
		s->files[p].first = PID_NONE;
		s->files[p].last  = PID_NONE;
	}
}

void init() {
	sched_init();

	// processus 0: p_idle
    proc_t *p_idle = &state.st_proc[PID_IDLE];
    p_idle->p_ppid = 0;
	p_idle->p_nchd = 1;
    p_idle->p_stat = RUN;
	p_idle->p_rng1 = true;
	p_idle->p_prio = 0; //priorité minimale
	p_idle->p_pml4 = (phy_addr)NULL;
	p_idle->p_reg.rsp = (uint_ptr)NULL; //pas de pile
	p_idle->p_reg.rip = (uint_ptr)&proc_idle_entry;

    // processus 1: init
    proc_t *p_init = &state.st_proc[PID_INIT];
    p_init->p_ppid = 1;
	p_init->p_nchd = 1;
    p_init->p_stat = RUN;
	p_init->p_rng1 = false; //TODO
	p_init->p_prio = NB_PRIORITY_LVL - 1; //priorité maximale

    // set file descriptors
    // stdin
    p_init->p_fds[0] = 0;
    state.st_chann[0].chann_mode = STREAM_IN;
    state.st_chann[0].chann_acc  = 1;

    // stdout
    p_init->p_fds[1] = 1;
    state.st_chann[1].chann_mode = STREAM_OUT;
    state.st_chann[1].chann_acc  = 1;

    // stderr
    p_init->p_fds[1] = 2;
    state.st_chann[2].chann_mode = STREAM_OUT;
    state.st_chann[2].chann_acc  = 1;

    // set unused file desc
    for (size_t i = 0; i < NFD; i++) p_idle->p_fds[i] = -1;
    for (size_t i = 3; i < NFD; i++) p_init->p_fds[i] = -1;

    // set chann to free
    for (cid_t cid = 3; cid < NCHAN; cid++) {
        state.st_chann[cid].chann_id   = cid;
        state.st_chann[cid].chann_mode = UNUSED;
    }

    // set all remaining slots to free processus
    for (pid_t pid = 2; pid < NPROC; pid++) {
        state.st_proc[pid].p_stat = FREE;
        state.st_proc[pid].p_ppid = 0;
	}

    vfs_init();

	sched_add_proc(PID_IDLE);
    proc_create_userspace(proc_init, p_init);

    state.st_curr_pid = 1;
    st_curr_reg       = &p_init->p_reg;

    klog(Log_info, "init", "Process 1 loaded. Start process 1");
	iret_to_proc(p_init);
}

void sched_add_proc(pid_t pid) {
	proc_t     *p = state.st_proc + pid;
	priority_t pr = p->p_prio;
	pid_t     prl = state.st_sched.files[pr].last;
	
	klogf(Log_verb, "sched", "add %d @ %d", (int)pid, (int)pr);

	++state.st_sched.nb_proc;
	p->p_nxpf = PID_NONE;
	if (~prl)
		state.st_proc[prl].p_nxpf = pid;
	else {
		state.st_sched.files[pr].first = pid;
		state.st_sched.pres |= ((uint64_t)1) << pr;
	}
	state.st_sched.files[pr].last = pid;
}

pid_t sched_pop_proc() {
	struct scheduler* s = &state.st_sched;
	priority_t pr = find_bit_64(s->pres, 1, 6);
	pid_t     pid = s->files[pr].first;
	pid_t      nx = state.st_proc[pid].p_nxpf;
	klogf(Log_verb, "sched", "rem %d @ %d", (int)pid, (int)pr);
	--s->nb_proc;
	s->files[pr].first = nx;
	if (!~nx) {
		s->files[pr].last = PID_NONE;
		s->pres &= ~(((uint64_t)1) << pr);
	}
	return pid;
}

int hit = 0;

void schedule_proc() {
    if (state.st_sched.pres) {
		kAssert(state.st_sched.nb_proc > 0);
        // pick a new process to run
        pid_t pid = sched_pop_proc();
        proc_t *p = switch_proc(pid);

        klogf(Log_info, "sched",
              "nb R %d, run proc %d :\n"
			  "   rip %p, rsp %p",
              state.st_sched.nb_proc + 1, pid,
              p->p_reg.rip,
              p->p_reg.rsp);
		
		iret_to_proc(p);
    }
	
	// Le processus IDLE empêche que l'on arrive ici
	never_reached
}
pid_t schedule_proc_ev() {
	sched_add_proc(state.st_curr_pid);
	pid_t rt = sched_pop_proc();
	klogf(Log_verb, "sched",
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


static inline uint8_t proc_ldr_alloc_pages(uint_ptr begin, uint_ptr end) {
	return kmem_paging_alloc_rng(begin, end,
				PAGING_FLAG_U | PAGING_FLAG_W,
				PAGING_FLAG_U | PAGING_FLAG_W);
}

void proc_ldr_fill0(void* err_pt, Elf64_Addr dst, uint64_t sz) {
    uint8_t err = proc_ldr_alloc_pages(dst, dst + sz);
    if (err) {
		*((uint8_t*)err_pt) = err;
		return;
	}
    //TODO: use quad
    for (size_t i=0; i<sz; ++i)
        ((uint8_t*) dst)[i] = 0;
}

void proc_ldr_copy(void* err_pt, Elf64_Addr dst, void* src, uint64_t sz) {
    uint8_t err = proc_ldr_alloc_pages(dst, dst + sz);
    if (err) {
		*((uint8_t*)err_pt) = err;
		return;
	}
    for (size_t i=0; i<sz; ++i)
        ((uint8_t*) dst)[i] = ((uint8_t*) src)[i];
}

uint8_t proc_create_userspace(void* prg_elf, proc_t *proc) {
    struct elf_loader proc_ldr = {
        .fill0 = &proc_ldr_fill0,
        .copy  = &proc_ldr_copy
    };
    uint8_t err = 0;

    volatile phy_addr pml4_loc = kmem_alloc_page(); //TODO crash sans volatile
    kmem_bind_dynamic_slot(0, pml4_loc);

    kmem_init_pml4((uint64_t*)kmem_dynamic_slot(0), pml4_loc);
    clear_interrupt_flag();
    pml4_to_cr3(pml4_loc);
    set_interrupt_flag();

    //On est désormais dans le paging du processus
    err = 0;
	uint64_t* stack_pd = kmem_acc_pts_entry(paging_add_lvl(pgg_pd, USER_STACK_PD),
							2, PAGING_FLAG_U | PAGING_FLAG_W);
	*stack_pd = SPAGING_FLAG_P | PAGING_FLAG_W;
    proc->p_reg.rip = elf_load(proc_ldr, &err, prg_elf);
    if (err) return 2;

    proc->p_reg.rsp = paging_add_lvl(pgg_pd, USER_STACK_PD + 1);
    proc->p_pml4    = pml4_loc;

    return 0;
}

void proc_ps() {
	for (pid_t pid = 0; pid < NPROC; pid++) {
		proc_t* p = state.st_proc + pid;
		if (p->p_stat != FREE)
			kprintf("%cpid=%d st=%c ppid=%d pr=%d\n",
					pid==state.st_curr_pid ? '*' : ' ',
					(int)pid, (int)proc_state_char[p->p_stat],
					(int)p->p_ppid, (int)p->p_prio);
	}
}
