#ifndef _H_PROC
#define _H_PROC

#include "param.h"
#include "file.h"

typedef unsigned long size_t;
typedef int  pid_t;
typedef int  priority_t;

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

typedef struct proc {
    pid_t            p_pid;
    pid_t            p_ppid;     // parent pid
    enum proc_state  p_stat;     // current status of the processus
    int              p_pri;      // priority of the process in the heap
    int              p_fds[NFD]; // table of file descriptors
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
 * Associated syscalls
 */
pid_t wait();
pid_t fork();
void  kexit(int status);
void  sleep();

int open(char *fname, enum chann_mode);
int close(int fildes);
int write(int filedes, const void *buf, size_t len);
int read (int filedes, const void *buf, size_t len);

/**
 * Initialize state of the machine
 * and create process one
 */

void       init();
int        push_ps(pid_t pid);
pid_t      pop_ps();
int        waiting_ps();
proc_t     *gproc(pid_t pid);

#endif
