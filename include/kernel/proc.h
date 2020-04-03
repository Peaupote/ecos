#ifndef _H_PROC
#define _H_PROC

#include <headers/proc.h>

#include "kutil.h"
#include "int.h"
#include "gdt.h"
#include "param.h"
#include "file.h"
#include "memory/kmem.h"
#include "memory/shared_pages.h"

//Emplacement de la pile des processus
#define USER_STACK_PD 0x7FF

/**
 * Processus
 */

// Représentable sur 3 bits
enum proc_state {
    FREE,  // unused
    SLEEP, // sleeping
    WAIT,  // waiting
    BLOCK, // blocked by a syscall
    RUN,   // running
    ZOMB   // just terminated
};
extern char proc_state_char[6];

// offset to access in structure
#define RFL 0
#define RIP 8
#define RSP 16

typedef uint64_t reg_t;
struct reg {
    reg_t rflags, rip, rsp,
        rax, rbx, rcx, rdx, rsi,
        rdi, rbp, r8, r9, r10, r11,
        r12, r13, r14, r15;
} __attribute__((packed));

typedef struct proc {
    pid_t            p_ppid;     // parent pid
    union {
        pid_t        p_nxpf;     // if in a file: next in priority file
        pid_t        p_nxfr;     // if FREE: next free
        pid_t        p_nxzb;     // if ZOMB: next ZOMB sibling
        pid_t        p_nxwf;     // if BLOCK: next waiting file
    };
    pid_t            p_prsb;     // prev sibling
    pid_t            p_nxsb;     // next sibling
    pid_t            p_fchd;     // first child
    uint64_t         p_nchd;     // number of child processus
    //TODO: bit field
    enum proc_state  p_stat;     // current status of the processus
    uint8_t          p_ring;     // ring level (2 bits)
    priority_t       p_prio;     // priority of the process
    int              p_fds[NFD]; // table of file descriptors
    phy_addr         p_pml4;     // paging
    struct reg       p_reg;      // saved registers
} proc_t;

/**
 * Channels
 */

typedef struct channel {
    cid_t           chann_id;

    // pointer to virtual file
    // and current writing/reading position in the buffer
    vfile_t        *chann_vfile;
    off_t           chann_pos;

    enum chann_mode chann_mode; // kind of operations channel allowed
    uint64_t        chann_acc;  // number of times the channel is referenced
} chann_t;

// Contient les processus en mode RUN à l'exception du processus courant
// Ainsi que les processus BLOCK qui peuvent être repris
struct scheduler {
    // le bit p est set ssi un processus de priorité p est présent
    uint64_t    pres;
    // contient les processus de chaque priorités
    struct {
        pid_t   first;
        pid_t   last;
    } files[NB_PRIORITY_LVL];
    // nombre de processus dans la structure
    size_t      nb_proc;
};

/**
 * Global state of the machine
 */

struct {
    pid_t       st_curr_pid;          // pid of current running process
    struct scheduler st_sched;
    pid_t       st_free_proc;         // head of the free linked list
    proc_t      st_proc[NPROC];       // table containing all processes
    chann_t     st_chann[NCHAN];      // table containing all channels
    vfile_t     st_files[NFILE];      // table containing all opened files
} state;

// pointer to current proc registers
struct reg *st_curr_reg;

/**
 * Initialize state of the machine
 * create process IDLE and INIT
 * start INIT
 */
__attribute__ ((noreturn))
void proc_start(void);

void  sched_add_proc(pid_t);
// Le scheduler ne doit pas être vide
pid_t sched_pop_proc();

__attribute__ ((noreturn))
void  schedule_proc();
// Renvoi le nouveau processus
pid_t schedule_proc_ev();

//! change le paging vers celui du processus
//  les objets doivent se trouver dans l'espace du kernel
uint8_t proc_create_userspace(void* prg_elf, proc_t *proc);

// prennent en argument le sélecteur du CS destination
// (étendu à 8 octets par des zéros)
// le DS est CS + 8
__attribute__ ((noreturn))
extern void iret_to_userspace(uint64_t cs_ze);

__attribute__ ((noreturn))
extern void eoi_iret_to_userspace(uint64_t cs_ze);

extern reg_t continue_syscall();

__attribute__ ((noreturn))
static inline void iret_to_proc(const proc_t* p) {
    iret_to_userspace(gdt_ring_lvl(p->p_ring));
}

__attribute__ ((noreturn))
static inline void eoi_iret_to_proc(const proc_t* p) {
    eoi_iret_to_userspace(gdt_ring_lvl(p->p_ring));
}

__attribute__ ((noreturn))
static inline void run_proc(proc_t* p) {
    if (p->p_stat == RUN) iret_to_proc(p);
    else { //blocked
        p->p_stat = RUN;
        p->p_reg.rax = continue_syscall();
        iret_to_proc(p);
    }
}
__attribute__ ((noreturn))
static inline void eoi_run_proc(proc_t* p) {
    if (p->p_stat == RUN) eoi_iret_to_proc(p);
    else { //blocked
        p->p_stat = RUN;
        write_eoi();
        p->p_reg.rax = continue_syscall();
        iret_to_proc(p);
    }
}

proc_t *switch_proc(pid_t pid);

void proc_ps();

void proc_write_stdin(char *buf, size_t len);
void wait_file(pid_t pid, vfile_t *file);
//

static inline pid_t find_new_pid() {
    pid_t pid = state.st_free_proc;
    if (~pid)
        state.st_free_proc = state.st_proc[pid].p_nxfr;
    else
        klogf(Log_error, "proc", "can't find a new pid");
    return pid;
}

static inline void free_pid(pid_t p) {
    state.st_proc[p].p_nxfr = state.st_free_proc;
    state.st_free_proc = p;
    state.st_proc[p].p_stat = FREE;
}

static inline void proc_set_curr_pid(pid_t pid) {
    state.st_curr_pid = pid;
    st_curr_reg = &state.st_proc[pid].p_reg;
}


static inline uintptr_t make_proc_stack() {
    *kmem_acc_pts_entry(paging_add_lvl(pgg_pd, USER_STACK_PD),
                            2, PAGING_FLAG_U | PAGING_FLAG_W)
        = SPAGING_FLAG_P | PAGING_FLAG_W | PAGING_FLAG_U;
    return paging_add_lvl(pgg_pd, USER_STACK_PD + 1);
}

static inline cid_t free_chann() {
    cid_t cid;
    for (cid = 0; cid < NCHAN; cid++) {
        if (state.st_chann[cid].chann_mode == UNUSED)
            return cid;
    }

    return NCHAN;
}

#endif
