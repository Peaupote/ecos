#include <kernel/int.h>
#include <kernel/sys.h>
#include <kernel/proc.h>
#include <kernel/kutil.h>
#include <kernel/tty.h>

static struct {
    uint64_t sleep_counter;
    pid_t    pid;
} sleeps[NSLEEP];

static size_t nb_sleep = 0;

int sys_usleep(usecond_t time) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    kAssert(p->p_stat != SLEEP);
	
	uint64_t count = time * PIT_FREQ / 1000000;
	if (!count) return 0;

    // look for first empty spot
	if (nb_sleep >= NSLEEP) {
        klogf(Log_error, "syscall",
              "can't have more than %d processus sleeping", NSLEEP);
		set_errno(ENOMEM);
        return -1;
	}
    
	p->p_stat = SLEEP;
	size_t i = nb_sleep++;
    sleeps[i].pid = state.st_curr_pid;

    sleeps[i].sleep_counter = count;
    klogf(Log_info, "syscall", "process %d sleep for %lld sec",
            state.st_curr_pid, time);

    schedule_proc();
}

void lookup_end_sleep(void) {
    for (size_t i = 0; i < nb_sleep; ) {
        pid_t pid = sleeps[i].pid;
        if (state.st_proc[pid].p_stat == SLEEP &&
				--sleeps[i].sleep_counter == 0) {
			sleeps[i] = sleeps[--nb_sleep];
            state.st_proc[pid].p_stat = RUN;
			state.st_proc[pid].p_reg.r.rax.i = 0;
            sched_add_proc(pid);
        } else ++i;
    }
}
