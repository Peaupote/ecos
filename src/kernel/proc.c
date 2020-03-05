#include <stddef.h>
#include <stdint.h>

#include "proc.h"

#include "kmem.h"
#include "int.h"
#include "../util/elf64.h"

#define USER_STACK_TOP  0x57AC3000
#define USER_STACK_SIZE 0x4000

// priority in runqueue according to process status
int proc_state_pri[7] = { PFREE, PSLEEP, PWAIT, PRUN, PIDLE, PZOMB, PSTOP };

// some heap utilitary functions
// the priority queue is probably bugged
// has not been tested

int push_ps(pid_t pid) {
    // test if remain space
    if (state.st_waiting_ps == NHEAP)
        return -1;

    proc_t *p = &state.st_proc[pid];
    p->p_pri = proc_state_pri[p->p_stat];
    priority_t ps_priority = state.st_proc[pid].p_pri;
    state.st_runqueues[state.st_waiting_ps++] = pid;

    int i = state.st_waiting_ps;
    pid_t hp_pid = state.st_runqueues[i / 2 - 1];
    priority_t hp_priority = state.st_proc[hp_pid].p_pri;

    // up heap
    while (i / 2 > 0 && ps_priority > hp_priority) {
        state.st_runqueues[i - 1] = hp_pid;

        i /= 2;
        state.st_runqueues[i - 1] = pid;
        hp_pid = state.st_runqueues[i / 2 - 1];
        hp_priority = state.st_proc[hp_pid].p_pri;
    }

    return 0;
}

pid_t pop_ps() {
    // test if heap is non empty
    if (state.st_waiting_ps == 0)
        return -1;

    pid_t pid = state.st_runqueues[0];
    state.st_runqueues[0]  = state.st_runqueues[--state.st_waiting_ps];

    // down heap
    for (int i = 1; 2 * i < state.st_waiting_ps;) {
        int win = i, l = 2 * i, r = 2 * i + 1;

        pid_t pidwin = state.st_runqueues[win - 1],
            pidl = state.st_runqueues[l - 1],
            pidr = state.st_runqueues[r - 1];

        priority_t winp = state.st_proc[pidwin].p_pri,
            left  = state.st_proc[pidl].p_pri,
            right = state.st_proc[pidr].p_pri;

        if (left < winp)  { pidwin = pidl; winp = left; }
        if (right < winp) { pidwin = pidr; winp = right; }

        if (win == i) break;

        state.st_runqueues[win - 1] = state.st_runqueues[i];
        state.st_runqueues[i - 1]   = pidwin;
        i = win;
    }

    return pid;
}

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

    fp        = &state.st_proc[pid];
    fp->p_pid  = pid;
    fp->p_ppid = state.st_curr_pid;
    fp->p_stat = RUN;
    fp->p_pri  = p->p_pri;

    // copy file descriptors
    for (int i = 0; i < NFD; i++)
        fp->p_fds[i] = p->p_fds[i];

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

void init() {
    // construct processus one
    proc_t *one = &state.st_proc[1];
    one->p_pid  = 1;
    one->p_ppid = 0;
    one->p_stat = RUN;
    one->p_pc   = 0;

    one->p_reg.rax = 0;
    one->p_reg.rcx = 0;
    one->p_reg.rdx = 0;
    one->p_reg.rsi = 0;
    one->p_reg.rdi = 0;
    one->p_reg.r8 = 0;
    one->p_reg.r9 = 0;
    one->p_reg.r10 = 0;
    one->p_reg.r11 = 0;

    state.st_curr_pid     = 1;
    state.st_waiting_ps   = 0;

    // set all remaining slots to free processus
    for (pid_t pid = 2; pid < NPROC; pid++)
        state.st_proc[pid].p_stat = FREE;

    push_ps(1);
}


extern uint8_t dynamic_slot;
uint8_t err = 0;
void proc_ldr_alloc_pages(uint_ptr begin, uint_ptr end) {
	for(uint_ptr it = begin & PAGE_MASK; it < end; ++it)
		if(kmem_paging_alloc(it, PAGING_FLAG_U | PAGING_FLAG_R) >= 2)
			err = 1;
}
void proc_ldr_fill0(void* none __attribute__((unused)),
		Elf64_Addr dst, uint64_t sz) {
	proc_ldr_alloc_pages(dst, dst + sz);
	//TODO: use quad
	for (size_t i=0; i<sz; ++i)
		((uint8_t*) dst)[i] = 0;
}
void proc_ldr_copy(void* none __attribute__((unused)),
		Elf64_Addr dst, void* src, uint64_t sz) {
	proc_ldr_alloc_pages(dst, dst + sz);
	for (size_t i=0; i<sz; ++i)
		((uint8_t*) dst)[i] = ((uint8_t*) src)[i];
}

uint8_t proc_create_userspace(void* prg_elf, struct user_space* us,
		struct user_space_start* uss) {
	struct elf_loader proc_ldr = {
		.fill0 = &proc_ldr_fill0,
		.copy  = &proc_ldr_copy
	};

	volatile phy_addr pml4_loc = kmem_alloc_page(); //TODO crash sans volatile
	if(paging_force_map_to((uint_ptr)&dynamic_slot, pml4_loc))
		return 1;
	if(paging_phy_addr((uint_ptr)&dynamic_slot) != pml4_loc)// TODO rm TEST
		return 2;
	kmem_init_pml4((uint64_t*)&dynamic_slot, pml4_loc);
	clear_interrupt_flag();
	pml4_to_cr3(pml4_loc);
	set_interrupt_flag();

	//On est désormais dans le paging du processus
	err = 0;
	proc_ldr_alloc_pages(USER_STACK_TOP - USER_STACK_SIZE,
				USER_STACK_TOP);
	uss->entry = (void*)elf_load(proc_ldr, NULL, prg_elf);
	uss->rsp   = (void*)USER_STACK_TOP;
	us->pml4   = pml4_loc;

	if (err) return 3;

	return 0;
}
