#ifndef _H_PROC
#define _H_PROC

#include "param.h"
#include "file.h"
#include "kmem.h"

typedef int32_t  pid_t;
typedef int32_t  priority_t;

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
    enum proc_state  p_stat;     // current status of the processus
    priority_t       p_pri;      // priority of the process in the heap
    int              p_fds[NFD]; // table of file descriptors
    phy_addr         p_pml4;     // paging
    struct reg       p_reg;      // saved registers
} proc_t;

/**
 * Channels
 */

// TODO : could be encoded by 2 bits
enum chann_mode {
    UNUSED,
    READ,
    WRITE,
    RDWR
};

typedef struct channel {
    ino_t           chann_file; // file referenced by the channel
    enum chann_mode chann_mode; // kind of operations channel allow to perform
    unsigned int    chann_acc;  // number of times the channel is referenced
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
} state;

// pointer to current proc registers
struct reg *st_curr_reg;

/**
 * Initialize state of the machine
 * and create process one
 */
void init(void);
void schedule_proc(void);
pid_t push_ps(pid_t pid);

//! change le paging vers celui du processus
//  les objets doivent se trouver dans l'espace du kernel
uint8_t proc_create_userspace(void* prg_elf, proc_t *proc);

//! ne retournent pas à l'appelant
extern void iret_to_userspace(uint64_t rip, uint64_t rsp);
extern void eoi_iret_to_userspace(uint64_t rip, uint64_t rsp);

proc_t *switch_proc(pid_t pid);

#endif
