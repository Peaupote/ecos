#ifndef _H_PROC
#define _H_PROC

#include <stddef.h>
#include "param.h"
#include "file.h"
#include "memory/kmem.h"

//Emplacement de la pile des processus
#define USER_STACK_PD 0x7FF

typedef int32_t  pid_t;
typedef int32_t  priority_t;
typedef int32_t  cid_t;

/**
 * Processus
 */

// could be implemented using just 3 bits
enum proc_state {
    FREE,  // unused
    SLEEP, // sleeping
    WAIT,  // waiting
    RUN,   // running
    IDLE,  // just created
    ZOMB,  // just terminated
    STOP   // ...
};
extern char proc_state_char[7];

// offset to access in structure
#define RFL 0
#define RIP 8
#define RSP 16

struct reg {
    uint64_t rflags, rip, rsp,
        rax, rbx, rcx, rdx, rsi,
        rdi, rbp, r8, r9, r10, r11,
        r12, r13, r14, r15;
} __attribute__((packed));

typedef struct proc {
    pid_t            p_pid;
    pid_t            p_ppid;     // parent pid
    uint64_t         p_nchd;     // number of child processus
    enum proc_state  p_stat;     // current status of the processus
    priority_t       p_pri;      // priority of the process in the heap
    int              p_fds[NFD]; // table of file descriptors
    phy_addr         p_pml4;     // paging
    struct reg       p_reg;      // saved registers
} proc_t;

/**
 * Channels
 */

enum chann_mode {
    UNUSED,
    READ,
    WRITE,
    RDWR,
    STREAM_IN,
    STREAM_OUT
};

typedef struct channel {
    cid_t           chann_id;

    // pointer to virtual file
    // and current writing/reading position in the buffer
    vfile_t        *chann_vfile;
    off_t           chann_pos;

    enum chann_mode chann_mode; // kind of operations channel allowed
    uint64_t        chann_acc;  // number of times the channel is referenced
} chann_t;

/**
 * Global state of the machine
 */

struct {
    pid_t       st_curr_pid;          // pid of current running process
    pid_t       st_runqueues[NHEAP];  // queue of processes to run
    uint8_t     st_runqueues_lpr[(NHEAP + 7)/8];
    size_t      st_waiting_ps;        // number of processes in queue
    proc_t      st_proc[NPROC];       // table containing all processes
    chann_t     st_chann[NCHAN];      // table containing all channels
    vfile_t     st_files[NFILE];      // table containing all opened files
} state;

// pointer to current proc registers
struct reg *st_curr_reg;

/**
 * Initialize state of the machine
 * and create process one
 */
void init(void);
// Ne renvoie pas
void  schedule_proc();
// Renvoi le nouveau processus
pid_t schedule_proc_ev();
pid_t push_ps(pid_t pid);

//! change le paging vers celui du processus
//  les objets doivent se trouver dans l'espace du kernel
uint8_t proc_create_userspace(void* prg_elf, proc_t *proc);

//! ne retournent pas à l'appelant
// prennent en argument le sélecteur du CS destination 
// (étendu à 8 octets par des zéros)
// le DS est CS + 8
extern void iret_to_userspace(uint64_t cs_ze);
extern void eoi_iret_to_userspace(uint64_t cs_ze);

proc_t *switch_proc(pid_t pid);

void proc_ps();

static inline pid_t find_new_pid(pid_t search_p) {
    // TODO : find more efficient way to choose new pid
    pid_t pid;
    for (pid = search_p; pid < NPROC; pid++)
        if (state.st_proc[pid].p_stat == FREE) return pid;

	for (pid = 2; pid < search_p; pid++)
		if (state.st_proc[pid].p_stat == FREE) return pid;

	return search_p;
}


#endif
