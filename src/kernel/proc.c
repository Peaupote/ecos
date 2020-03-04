#include <stddef.h>
#include <stdint.h>

#include "int.h"
#include "sys.h"
#include "proc.h"

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

    state.st_waiting_ps--;
    return pid;
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
}


void schedule_proc(void) {
    pid_t pid = state.st_curr_pid;

    if (state.st_proc[pid].p_stat == RUN)
        push_ps(pid);

    if (state.st_waiting_ps > 0) {
        // pick a new process to run
        pid = pop_ps();
        state.st_curr_pid = pid;

        // TODO : jump ring 3 to exec process
        // for test purposes when not sleeping call sleep
        if (state.st_proc[pid].p_stat == RUN) {
            sleep(1);
        }
    }
}
