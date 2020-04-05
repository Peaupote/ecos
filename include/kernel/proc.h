#ifndef _H_PROC
#define _H_PROC

#include <headers/proc.h>
#include <headers/signal.h>

#include <util/misc.h>

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
    BLOCK, // blocked by a syscall
    BLOCR, // blocked by a syscall, in file to return
    RUN,   // running
    ZOMB   // just terminated
};
extern char proc_state_char[6];

// offset to access in structure
#define RFL 0
#define RIP 8
#define RSP 16

typedef uint64_t reg_t;
struct base_reg { // sz = 17 * 8 = 8 [16]
	reg_t rip, rsp,
	rax, rbx, rcx, rdx, rsi,
	rdi, rbp, r8, r9, r10, r11,
	r12, r13, r14, r15;
} __attribute__((packed));
struct reg { // sz = 18 * 8 = 0 [16]
    reg_t rflags;
	struct base_reg b;
} __attribute__((packed));

struct proc_shnd {
	sigset_t         blk;     // blocked signals: 0 = blocked
	sigset_t         ign;     // 1 = ignore signal
	sigset_t         dfl;     // 1 = default signal disposition
	sighandler_t     usr;     // user sig handler
};

typedef struct proc {
    pid_t            p_ppid;     // parent pid
    union {
        pid_t        p_nxpf;     // if in a file (BLOCK|BLOKR):
		                         //               next in priority file
        pid_t        p_nxwl;     // if BLOCK:     next waiting list
        pid_t        p_nxfr;     // if FREE:      next free
        pid_t        p_nxzb;     // if ZOMB:      next ZOMB sibling
    };
	union {
		pid_t*       p_prpf;     // if in a file: ref in priority file
		pid_t*       p_prwl;     // if BLOCK:     !=NULL, ref in waiting list
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
	uint_ptr         p_brk;      // program break
    struct reg       p_reg;      // saved registers
    char             p_cmd[256]; // cmd to exec the process

    ino_t            p_cino;     // inode of current dir
    dev_t            p_dev;      // device of current dir

	sigset_t         p_spnd;     // pending signals
	struct proc_shnd p_shnd;
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

// Contient les processus en mode RUN ou BLOCR 
// à l'exception du processus courant
struct scheduler_file {
	pid_t   first;
	pid_t   last;
};
struct scheduler {
    // le bit p est set ssi un processus de priorité p est présent
    uint64_t    pres;
    // contient les processus de chaque priorités
    struct scheduler_file files[NB_PRIORITY_LVL];
    // nombre de processus dans la structure
    size_t      nb_proc;
};

/**
 * Global state of the machine
 */

struct {
    pid_t       st_curr_pid;          // pid of current running process
                                      //   should be RUN or BLOCR 
    struct scheduler st_sched;
    pid_t       st_free_proc;         // head of the free linked list
    proc_t      st_proc[NPROC];       // table containing all processes
    chann_t     st_chann[NCHAN];      // table containing all channels
    vfile_t     st_files[NFILE];      // table containing all opened files
} state;

// pointer to current proc registers
struct reg *st_curr_reg;

static inline proc_t* cur_proc() {
	return state.st_proc + state.st_curr_pid;
}

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
void  schedule_proc(); // TODO: reset rsp to avoid stack overflow
// Renvoi le nouveau processus
pid_t schedule_proc_ev();

//! change le paging vers celui du processus
//  les objets doivent se trouver dans l'espace du kernel
uint8_t proc_create_userspace(void* prg_elf, proc_t *proc);

// 0 <= sigid < SIG_COUNT
void proc_hndl_sig_i(int sigid);
static inline void proc_hndl_sigs() {
	proc_t* p = cur_proc();
	while(p->p_spnd & p->p_shnd.blk)
		proc_hndl_sig_i(
				find_bit_32(p->p_spnd & p->p_shnd.blk, 1, 5));
}

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
    if (p->p_stat == RUN)
		iret_to_proc(p);
    else { //BLOCR
        p->p_stat = RUN;
        p->p_reg.b.rax = continue_syscall();
        iret_to_proc(p);
    }
}

proc_t *switch_proc(pid_t pid);

void proc_ps();

void proc_write_stdin(char *buf, size_t len);
__attribute__((noreturn))
void wait_file(pid_t pid, vfile_t *file);

// Retourne true ssi une action (kill...) a été effectué immédiatement
// sur le processus cible
// Peut modifier le processus courant
bool send_sig_to_proc(pid_t pid, int sigid);

__attribute__((noreturn))
void kill_proc_nr(int status);
void kill_proc(int status);

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

// Enlève un processus BLOCK de la liste où il se trouve
static inline void proc_extract_blk(proc_t* p) {
	*p->p_prwl = p->p_nxwl;
}

static inline void proc_blocr(pid_t pid, proc_t* p) {
	p->p_stat  = BLOCR;
	sched_add_proc(pid);
}

static inline void proc_unblock_1(pid_t pid, proc_t* p) {
	proc_extract_blk(p);
	proc_blocr(pid, p);
}

static inline void proc_unblock_list(pid_t* first) {
	proc_t* p;
	klogf(Log_error, "unblk", "%p = %d", first, *first);
	for (pid_t pid = *first; ~pid;) {
		p = state.st_proc + pid;
		p->p_stat  = BLOCR;
		pid_t npid = p->p_nxwl;
		sched_add_proc(pid);
		pid = npid;
	}
	*first = PID_NONE;
}

// Enlève un processus RUN ou BLOCR de la file où il se trouve
static inline void proc_extract_pf(proc_t* p) {
	*p->p_prpf = p->p_nxpf;
	if (!~p->p_nxpf && !~state.st_sched.files[p->p_prio].first)
		clear_bit_64(&state.st_sched.pres, p->p_prio);

#ifdef __is_debug
	p->p_prpf = NULL;
#endif
}

static inline void proc_self_block(proc_t* p) {
	p->p_stat = BLOCK;
	p->p_nxwl = PID_NONE;
	p->p_prwl = &p->p_nxwl;
}

#endif
