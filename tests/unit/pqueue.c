#include <stddef.h>
#include <stdint.h>

#include "tutil.h"

#include "../../src/kernel/param.h"

typedef size_t pid_t;
typedef int32_t  priority_t;

enum proc_state {
    FREE, SLEEP, WAIT, RUN, IDLE, ZOMB, STOP
};
typedef struct {
    enum proc_state p_stat;
	priority_t p_pri;
} proc_t;
struct {
    pid_t      st_runqueues[NHEAP];
	uint8_t    st_runqueues_lpr[(NHEAP + 7)/8];
	size_t     st_waiting_ps;
    proc_t     st_proc[NPROC];
} state;

#include "PQUEUE@src__kernel__proc.c.tst"
//void push_ps(pid_t)
//pid_t pop_ps()

void print_runqueue() {
	test_infoi("content: %d\n", state.st_waiting_ps);
	for (size_t i = 0; i < state.st_waiting_ps; ++i){
		test_infoi("  %d:", i+1);
		test_infoi("%d", state.st_runqueues[i]);
		test_infoi("(%d)", state.st_proc[state.st_runqueues[i]].p_pri);
	}
	test_infoi("\n",0);
}

int main() {
	test_init("pqueue");
	for (size_t i = 0; i < NPROC; ++i)
		state.st_proc[i].p_stat = (enum proc_state)rand_rng(0, 7);
	for (size_t i = 0; i < NHEAP; ++i)
		if (push_ps(i)) {
			test_fail("push return non 0");
			return 1;
		}
	pid_t out[NHEAP];
	size_t nb[NHEAP] = {0};
	for (size_t i = 0; i < NHEAP; ++i) {
		pid_t rt = pop_ps();
		if (rt == (pid_t)-1) {
			test_fail("pop return -1");
			return 1;
		} else if(rt >= NHEAP) {
			test_fail("pop non present");
			return 1;
		} else if(nb[rt]++) {
			test_fail("deja pop");
			return 1;
		}
		out[i] = rt;
	}
	for(size_t i=1; i<NHEAP; ++i){
		if(state.st_proc[out[i-1]].p_pri 
				> state.st_proc[out[i]].p_pri) {
			test_fail("bad order");
			return 1;
		}
	}
	return 0;
}
