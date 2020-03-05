#include "../int.h"
#include "../sys.h"
#include "../proc.h"

#define NSLEEP 128

struct {
    uint32_t sleep_counter;
    pid_t    pid;
} sleeps[NSLEEP];

void sleep_hdl(uint32_t time) {
    pid_t pid = state.st_curr_pid;
    size_t i = 0;

    // look for first empty spot
    while(i < NSLEEP && state.st_proc[sleeps[i].pid].p_stat == SLEEP) i++;
    if (i == NSLEEP) {
        // TODO handle error
        return;
    }

    state.st_proc[pid].p_stat = SLEEP;
    sleeps[i].pid = pid;

    // probably not correct here
    sleeps[i].sleep_counter = time * (1193180 / (1L << 16));
}

void lookup_end_sleep(void) {
    for (size_t i = 0; i < NSLEEP; i++) {
        pid_t pid = sleeps[i].pid;
        if (state.st_proc[pid].p_stat == SLEEP) {
            if (--sleeps[i].sleep_counter == 0) {
                state.st_proc[pid].p_stat = RUN;
                push_ps(pid);
            }
        }

    }
}
