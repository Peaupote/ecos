#include <kernel/proc.h>

#include <stddef.h>
#include <stdint.h>

#include <kernel/int.h>
#include <kernel/sys.h>

#include <libc/sys.h>
#include <libc/stdio.h>
#include <libc/string.h>
#include <util/elf64.h>
#include <kernel/memory/kmem.h>
#include <kernel/int.h>
#include <kernel/tty.h>
#include <kernel/kutil.h>
#include <kernel/gdt.h>
#include <util/misc.h>
#include <fs/proc.h>

char proc_state_char[6] = {'f', 's', 'w', 'b', 'R', 'Z'};

extern void proc_idle_entry(void);
extern void proc_init_entry(void);

void sched_init() {
    struct scheduler* s = &state.st_sched;
    s->pres    = 0;
    s->nb_proc = 0;
    for (size_t p = 0; p < NB_PRIORITY_LVL; ++p) {
        s->files[p].first = PID_NONE;
        s->files[p].last  = PID_NONE;
    }
}

void proc_start() {
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
    p_idle->p_brk  = 0;
    p_idle->p_reg.rsp = (uint_ptr)kernel_stack_top;
    p_idle->p_reg.rip = (uint_ptr)&proc_idle_entry;

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
    p_init->p_brk  = 0x1000;
    p_init->p_reg.rsp = (uint_ptr)NULL;
    p_init->p_reg.rip = (uint_ptr)&proc_init_entry;
    strcpy(p_init->p_cmd, "init");

    // processus 2: stop, utilisé en cas de panic
    proc_t *p_stop = &state.st_proc[PID_STOP];
    p_stop->p_ppid = PID_STOP;
    p_stop->p_prsb = p_stop->p_nxsb = PID_NONE;
    p_stop->p_fchd = PID_STOP;
    p_stop->p_nchd = 1;
    p_stop->p_stat = WAIT;
    p_stop->p_ring = 0;
    p_stop->p_prio = NB_PRIORITY_LVL - 1;
    p_stop->p_pml4 = (phy_addr)NULL;
    p_stop->p_brk  = 0;
    p_stop->p_reg.rsp = (uint_ptr)kernel_stack_top;
    p_stop->p_reg.rip = (uint_ptr)&proc_idle_entry;
    strcpy(p_stop->p_cmd, "stop");

    // set unused file desc
    for (size_t i = 0; i < NFD; i++)
        p_idle->p_fds[i] = p_stop->p_fds[i] = -1;
    for (size_t i = 3; i < NFD; i++) p_init->p_fds[i] = -1;

    // set chann to free
    for (cid_t cid = 3; cid < NCHAN; cid++) {
        state.st_chann[cid].chann_id   = cid;
        state.st_chann[cid].chann_mode = UNUSED;
        state.st_chann[cid].chann_waiting = -1;
    }

    // set all remaining slots to free processus
    state.st_free_proc = 3;
    for (pid_t pid = 3; pid < NPROC; pid++) {
        state.st_proc[pid].p_stat = FREE;
        state.st_proc[pid].p_nxfr = pid + 1;
    }
    state.st_proc[NPROC - 1].p_nxfr = PID_NONE;

    vfs_init();

    sched_add_proc(PID_IDLE);

    state.st_curr_pid = PID_INIT;
    st_curr_reg       = &p_init->p_reg;

    proc_create(1);
    proc_create(2);
    proc_alloc_std_streams(1);

    klogf(Log_info, "init", "Start process init @ %p", p_init->p_reg.rip);
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

        run_proc(p);
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

void proc_ldr_fill0(void* err_pt, Elf64_Xword flag __attribute__((unused)),
        Elf64_Addr dst, uint64_t sz) {
    uint8_t err = proc_ldr_alloc_pages(dst, dst + sz);
    if (err) {
        *((uint8_t*)err_pt) = err;
        return;
    }
    //TODO: use quad
    for (size_t i=0; i<sz; ++i)
        ((uint8_t*) dst)[i] = 0;
}

void proc_ldr_copy(void* err_pt, Elf64_Xword flag __attribute__((unused)),
        Elf64_Addr dst, void* src, uint64_t sz) {
    uint8_t err = proc_ldr_alloc_pages(dst, dst + sz);
    if (err) {
        *((uint8_t*)err_pt) = err;
        return;
    }
    for (size_t i=0; i<sz; ++i)
        ((uint8_t*) dst)[i] = ((uint8_t*) src)[i];
}

//UNUSED
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
    proc->p_reg.rsp = make_proc_stack();

    err = 0;
    proc->p_reg.rip = elf_load(proc_ldr, &err, prg_elf);
    if (err) return 2;

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
                    (int)p->p_ppid, 19 - (int)p->p_prio);
    }
}

void proc_write_stdin(char *buf, size_t len) {
    // TODO : handle differently to dont block other processes
    vfile_t *vf = vfs_load("/proc/1/fd/0");
    if (!vf) return;

    if (vfs_write(vf, buf, 0, len) < 0) {
        klogf(Log_error, "proc", "problem when writing on stdin");
    }

    vfs_close(vf);
}

void wait_file(pid_t pid, cid_t cid) {
    klogf(Log_info, "proc", "pid %d waiting for channel %d", pid, cid);
    proc_t *p = state.st_proc + pid;
    chann_t *c = state.st_chann + cid;
    vfile_t *vf = c->chann_vfile;

    p->p_stat = BLOCK;
    p->p_nxwf = c->chann_waiting;
    c->chann_waiting = pid;

    c->chann_nxw   = vf->vf_waiting;
    vf->vf_waiting = cid;

    schedule_proc();
    never_reached;
}
