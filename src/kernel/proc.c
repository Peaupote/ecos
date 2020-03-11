#include <stddef.h>
#include <stdint.h>

#include "int.h"
#include "sys.h"
#include "proc.h"

#include "memory/kmem.h"
#include "int.h"
#include "tty.h"
#include "../util/elf64.h"
#include "kutil.h"

#define USER_STACK_TOP  0x57AC3000
#define USER_STACK_SIZE 0x4000

#define TEST_SECTION_PQUEUE

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
            ps_priority < (hp_priority = state.st_proc[hp_pid].p_pri))) {
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
    // sort imédiatement si st_waiting_ps <= 1
    for (size_t i = 1; (i<<1) <= state.st_waiting_ps;) {
        size_t  win = i, l = i<<1, r = (i<<1) + 1;
        size_t  lpr_index = i - 1;
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
            priority_t rp = state.st_proc[pidr].p_pri;
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

        rq1[win] = rq1[i];
        rq1[i]   = pidwin;
        i = win;
    }

    return pid;
}

#undef TEST_SECTION_PQUEUE

extern uint8_t proc_init[];

void init() {
    // construct processus one
    proc_t *one = &state.st_proc[1];
    one->p_pid   = 1;
    one->p_ppid  = 0;
    one->p_stat  = RUN;
    one->p_pml4  = 0;
    one->p_nchd  = 0;

    // set file descriptors
    // stdin
    one->p_fds[0] = 0;
    state.st_chann[0].chann_mode = STREAM_IN;
    state.st_chann[0].chann_acc  = 1;

    // stdout
    one->p_fds[1] = 1;
    state.st_chann[1].chann_mode = STREAM_OUT;
    state.st_chann[1].chann_acc  = 1;

    // stderr
    one->p_fds[1] = 2;
    state.st_chann[2].chann_mode = STREAM_OUT;
    state.st_chann[2].chann_acc  = 1;

    // set unused file desc
    for (size_t i = 3; i < NFD; i++) one->p_fds[i] = -1;

    // set chann to free
    for (cid_t cid = 3; cid < NCHAN; cid++)
        state.st_chann[cid].chann_mode = UNUSED;

    // set all remaining slots to free processus
    for (pid_t pid = 2; pid < NPROC; pid++) {
        state.st_proc[pid].p_stat = FREE;
        state.st_proc[pid].p_ppid = 0;
    }

    proc_create_userspace(proc_init, one);

    state.st_curr_pid   = 1;
    state.st_waiting_ps = 0;
    st_curr_reg = &one->p_reg;

    klog(Log_info, "init", "Process 1 loaded. Start process 1");
    iret_to_userspace(one->p_reg.rip, one->p_reg.rsp);
}


void schedule_proc(uint8_t loop) {
    clear_interrupt_flag();
    if (state.st_waiting_ps > 0) {
        // pick a new process to run
        pid_t pid = pop_ps();
        proc_t *p = switch_proc(pid);

        klogf(Log_info, "sched",
              "nb waiting %d\n"
              "run proc %d : rip %h, rsp %h",
              state.st_waiting_ps + 1, pid,
              p->p_reg.rip,
              p->p_reg.rsp);

        eoi_iret_to_userspace(p->p_reg.rip, p->p_reg.rsp);
    } else if (loop && state.st_proc[state.st_curr_pid].p_stat != RUN) {
        // no process to take hand
        set_interrupt_flag();
        while(1) halt();
    }
}

proc_t *switch_proc(pid_t pid) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    if (p->p_pid != pid && p->p_stat == RUN)
        push_ps(state.st_curr_pid);

    p = &state.st_proc[pid];
    state.st_curr_pid = pid;
    st_curr_reg = &p->p_reg;
    pml4_to_cr3(p->p_pml4);

    return p;
}

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
	kmem_bind_dynamic_slot(0, pml4_loc);

    kmem_init_pml4((uint64_t*)kmem_dynamic_slot(0), pml4_loc);
    clear_interrupt_flag();
    pml4_to_cr3(pml4_loc);
    set_interrupt_flag();

    //On est désormais dans le paging du processus
    err = 0;
    proc_ldr_alloc_pages(USER_STACK_TOP - USER_STACK_SIZE,
                         USER_STACK_TOP);
    proc->p_reg.rip = elf_load(proc_ldr, &err, prg_elf);
    if (err) return 2;

    proc->p_reg.rsp = USER_STACK_TOP;
    proc->p_pml4    = pml4_loc;

    return 0;
}
