#include <kernel/int.h>
#include <kernel/sys.h>
#include <kernel/proc.h>
#include <kernel/kutil.h>
#include <kernel/tty.h>

struct {
    uint32_t sleep_counter;
    pid_t    pid;
} sleeps[NSLEEP];

uint64_t sleep(uint64_t time) {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    size_t i = 0;

    if (p->p_stat == SLEEP)
        kpanic("try to make sleep a sleeping process");

    // look for first empty spot
    while(i < NSLEEP && state.st_proc[sleeps[i].pid].p_stat == SLEEP) i++;
    if (i == NSLEEP) {
        // TODO handle error
        klogf(Log_error, "syscall",
              "can't have more than %d processus sleeping", NSLEEP);
        return ~(uint64_t)0;
    }

    p->p_stat = SLEEP;
    sleeps[i].pid = state.st_curr_pid;

    // probably not correct here
    sleeps[i].sleep_counter = time * (1193180 / (1L << 16));
    klogf(Log_info, "syscall", "process %d sleep for %d sec",
            state.st_curr_pid, time);

    schedule_proc();
    never_reached return 0;
}

void lookup_end_sleep(void) {
    for (size_t i = 0; i < NSLEEP; i++) {
        pid_t pid = sleeps[i].pid;
        if (state.st_proc[pid].p_stat == SLEEP &&
            --sleeps[i].sleep_counter == 0) {
            state.st_proc[pid].p_stat = RUN;
            sched_add_proc(pid);
        }
    }
}
