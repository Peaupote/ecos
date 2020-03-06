#include <stddef.h>
#include <stdint.h>

#include "int.h"
#include "sys.h"
#include "proc.h"

#include "kmem.h"
#include "int.h"
#include "tty.h"
#include "../util/elf64.h"
#include "kutil.h"

#define USER_STACK_TOP  0x57AC3000
#define USER_STACK_SIZE 0x4000

// priority in runqueue according to process status
int proc_state_pri[7] = { PFREE, PSLEEP, PWAIT, PRUN, PIDLE, PZOMB, PSTOP };

// some heap utilitary functions

int push_ps(pid_t pid) {
    // test if remain space
    if (state.st_waiting_ps >= NHEAP)
        return -1;

    pid_t* rq1 = state.st_runqueues - 1;
    proc_t *p = state.st_proc + pid;
    priority_t ps_priority = p->p_pri = proc_state_pri[p->p_stat];
    size_t i = ++state.st_waiting_ps;

    pid_t hp_pid = rq1[i >> 1];
    priority_t hp_priority = state.st_proc[hp_pid].p_pri;

    // up heap
    while ((i>>1)
        && (hp_pid = rq1[i>>1],
            ps_priority > (hp_priority = state.st_proc[hp_pid].p_pri))) {
        rq1[i] = hp_pid;
        i >>= 1;
    }
    rq1[i] = pid;
    return 0;
}

pid_t pop_ps() {
    // test if heap is non empty
    if (state.st_waiting_ps == 0)
        return -1;

    pid_t* rq1 = state.st_runqueues - 1;
    pid_t pid  = rq1[1];
    rq1[1]     = rq1[state.st_waiting_ps--];

    // down heap
    for (int i = 1; (i<<1) <= state.st_waiting_ps;) {
        int win = i, l = i<<1, r = (i<<1) + 1;
        size_t lpr_index = i - 1;
        uint8_t lpr_shift = 1 << (lpr_index % 8);
        lpr_index /= 8;

        pid_t pidwin = rq1[win],
                pidl = rq1[l];

        priority_t winp = state.st_proc[pidwin].p_pri,
                   lp   = state.st_proc[pidl].p_pri;

        if (lp < winp) {
            win    = l;
            pidwin = pidl;
            winp   = lp;
        }
        if (r <= state.st_waiting_ps){
            pid_t pidr    = rq1[r];
            priority_t rp = state.st_proc[r].p_pri;
            if (rp < winp ||
                (rp == winp
                 && (lpr_shift &
                     (state.st_runqueues_lpr[lpr_index] ^= lpr_shift)))) {
                win    = r;
                pidwin = pidr;
                winp   = lp;
            }
        }

        if (win == i) break;

        rq1[win] = state.st_runqueues[i];
        state.st_runqueues[i - 1]   = pidwin;
        i = win;
    }

    return pid;
}

extern uint8_t proc_init[];

void init() {
    // construct processus one
    proc_t *one = &state.st_proc[1];
    one->p_pid   = 1;
    one->p_ppid  = 0;
    one->p_stat  = RUN;
    one->p_pml4  = 0;
    one->p_rip   = 0;
    one->p_rsp   = 0;

    one->p_reg.rax = 0;
    one->p_reg.rcx = 0;
    one->p_reg.rdx = 0;
    one->p_reg.rsi = 0;
    one->p_reg.rdi = 0;
    one->p_reg.r8 = 0;
    one->p_reg.r9 = 0;
    one->p_reg.r10 = 0;
    one->p_reg.r11 = 0;

    proc_create_userspace(proc_init, one);

    state.st_curr_pid   = 1;
    state.st_waiting_ps = 0;

    // set all remaining slots to free processus
    for (pid_t pid = 2; pid < NPROC; pid++)
        state.st_proc[pid].p_stat = FREE;
}


void schedule_proc(void) {
    pid_t pid = state.st_curr_pid;

    if (state.st_proc[pid].p_stat == RUN)
        push_ps(pid);

    if (state.st_waiting_ps > 0) {
        // pick a new process to run
        pid = pop_ps();
        state.st_curr_pid = pid;

        klogf(Log_info, "sched",
             "nb waiting %d\n"
             "proc %d : rip %h, rsp %h",
				state.st_waiting_ps, pid,
                state.st_proc[pid].p_rip,
                state.st_proc[pid].p_rsp);

        eoi_iret_to_userspace(state.st_proc[pid].p_rip,
                          state.st_proc[pid].p_rsp);
    }
}


extern uint8_t dynamic_slot;

uint8_t proc_ldr_alloc_pages(uint_ptr begin, uint_ptr end) {
	uint8_t err;
    for(uint_ptr it = begin & PAGE_MASK; it < end; ++it) {
		err = kmem_paging_alloc(it, PAGING_FLAG_U | PAGING_FLAG_R);
        if(err >= 2) return err;
	}
	return 0;
}

void proc_ldr_fill0(void* err_pt, Elf64_Addr dst, uint64_t sz) {
    uint8_t err = proc_ldr_alloc_pages(dst, dst + sz);
	if (err) *((uint8_t*)err_pt) = err;
    //TODO: use quad
    for (size_t i=0; i<sz; ++i)
        ((uint8_t*) dst)[i] = 0;
}

void proc_ldr_copy(void* err_pt, Elf64_Addr dst, void* src, uint64_t sz) {
    uint8_t err = proc_ldr_alloc_pages(dst, dst + sz);
	if (err) *((uint8_t*)err_pt) = err;
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
    if(paging_force_map_to((uint_ptr)&dynamic_slot, pml4_loc))
        return 1;

    kmem_init_pml4((uint64_t*)&dynamic_slot, pml4_loc);
    clear_interrupt_flag();
    pml4_to_cr3(pml4_loc);
    set_interrupt_flag();

    //On est désormais dans le paging du processus
    err = 0;
    proc_ldr_alloc_pages(USER_STACK_TOP - USER_STACK_SIZE,
                         USER_STACK_TOP);
    proc->p_rip = (void*)elf_load(proc_ldr, &err, prg_elf);
	if (err) return 2;

    proc->p_rsp   = (void*)USER_STACK_TOP;
    proc->p_pml4  = pml4_loc;

    return 0;
}
