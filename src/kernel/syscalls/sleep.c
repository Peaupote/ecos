#include "../int.h"
#include "../sys.h"
#include "../proc.h"
#include "../kutil.h"
#include "../tty.h"

struct {
    uint32_t sleep_counter;
    pid_t    pid;
} sleeps[NSLEEP];

void sleep() {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    size_t i = 0;
    uint64_t time = p->p_reg.rdi;

    if (p->p_stat == SLEEP) {
        kpanic("try to make sleep a sleeping process");
    }

    // look for first empty spot
    while(i < NSLEEP && state.st_proc[sleeps[i].pid].p_stat == SLEEP) i++;
    if (i == NSLEEP) {
        // TODO handle error
        klogf(Log_error, "syscall",
              "can't have more than %d processus sleeping", NSLEEP);
        return;
    }

    p->p_stat = SLEEP;
    sleeps[i].pid = p->p_pid;

    // probably not correct here
    sleeps[i].sleep_counter = time * (1193180 / (1L << 16));
    klogf(Log_info, "syscall", "process %d sleep for %d sec", p->p_pid, time);
    schedule_proc(1);
}

void lookup_end_sleep(void) {
    for (size_t i = 0; i < NSLEEP; i++) {
        pid_t pid = sleeps[i].pid;
        if (state.st_proc[pid].p_stat == SLEEP &&
            --sleeps[i].sleep_counter == 0) {
            state.st_proc[pid].p_stat = RUN;
            push_ps(pid);
        }
    }
}
