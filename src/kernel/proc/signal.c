#include <headers/signal.h>
#include <headers/sys.h>

#include <kernel/proc.h>
#include <kernel/sys.h>

#include <libc/errno.h>

#include <util/misc.h>

static inline bool contain_id(sigset_t set, int sigid) {
	return (set >> sigid) & 1;
}

void sys_sigsethnd(sighandler_t h) {
	cur_proc()->p_shnd.usr = h;
}

uint8_t sys_signal(int sigid, uint8_t hnd) {
	if (sigid < 0 || sigid >= SIG_COUNT || contain_id(SIG_NOTCTB, sigid))
			return ~(uint8_t)0;
	proc_t* p = cur_proc();
	int    rt = (contain_id(p->p_shnd.ign, sigid) ? 1 : 0)
			  | (contain_id(p->p_shnd.dfl, sigid) ? 2 : 0);
	p->p_shnd.ign = (p->p_shnd.ign & ~(((sigset_t)1) << sigid))
				  | (((sigset_t)(hnd & 0x1)) << sigid);
	p->p_shnd.dfl = (p->p_shnd.dfl & ~(((sigset_t)1) << sigid))
				  | (((sigset_t)((hnd>>1) & 0x1)) << sigid);
	return rt;
}

struct shnd_save { //sz = 2 * 4 + (2 + 9) * 8 = 12 * 8 = 0 [16]
	sigset_t blocked;
    uint32_t      :32;
	reg_t    rip, rsp;
	struct reg_rsv r;
} __attribute__((packed));

void sys_sigreturn() {
	proc_t *p = cur_proc();
	struct shnd_save* sv = (struct shnd_save*) p->p_reg.rsp.p;
	p->p_shnd.blk = sv->blocked | SIG_NOTCTB;
	p->p_reg.rip = sv->rip;
	p->p_reg.rsp = sv->rsp;
	p->p_reg.r   = sv->r;
	proc_hndl_sigs();
	iret_to_proc(p);
}

static inline void abort_syscalls(proc_t* p) {
	if (p->p_stat == BLOCR) {
		// Si un appel système est en cours il est interrompu et renvoi -1
		p->p_stat         = RUN;
		p->p_reg.r.rax.rd = ~(reg_data_t)0;
		set_proc_errno(p, EINTR);
	}
}

void proc_hndl_sig_i(int sigid) {
	klogf(Log_info, "sig", "hndl: i%d -> %d", sigid, state.st_curr_pid);
	proc_t* p  = cur_proc();
	p->p_spnd &= ~(((sigset_t)1) << sigid);

	if ((p->p_shnd.ign >> sigid) & 1)
		return;

	if (contain_id(p->p_shnd.dfl, sigid)) {

		if ((SIG_DFLKIL >> sigid) & 1)
			kill_proc_nr((sigid+1) * 0x100);

		if (contain_id(SIG_DFLSTP, sigid)) {
			abort_syscalls(p);
			p->p_stat = STOP;
			schedule_proc();
		}

		return; // pas d'action par défaut à ce stade
	}

	// User handler

	abort_syscalls(p);

	uint_ptr sv_loc = (uint_ptr)p->p_reg.rsp.p;
	sv_loc &= ~(uint_ptr)0xf; //align 16
	sv_loc -= sizeof(struct shnd_save);
	struct shnd_save* sv = (struct shnd_save*) sv_loc;
	sv->rip        = p->p_reg.rip;
	sv->rsp        = p->p_reg.rsp;
	sv->r          = p->p_reg.r;
	sv->blocked    = p->p_shnd.blk;
	p->p_shnd.blk &= ~(((sigset_t) 1) << sigid);
	
	p->p_reg.rsp.p   = (void*) sv_loc;// = 0 [16]
	p->p_reg.r.rdi.i = sigid;
	p->p_reg.rip.p   = p->p_shnd.usr;

	iret_to_proc(p);
}

int8_t send_sig_to_proc(pid_t pid, int sigid) {
	proc_t* p  = state.st_proc + pid;
	klogf(Log_info, "sig", "send i%d to %d %c%c",
			sigid, (int)pid,
			contain_id(p->p_shnd.ign, sigid) ? 'i' : '_',
			contain_id(p->p_shnd.dfl, sigid) ? 'd' : '_');
	if (sigid + 1 == SIGKILL) {

		if (state.st_curr_pid != pid) {
			switch (p->p_stat) {
				case BLOCK:
					if (p->p_reg.r.rax.ll == SYS_EXECVE) {
						pid_t aux_proc = p->p_reg.r.rdi.pid_t;
						proc_execve_abort(aux_proc);
					} else
						proc_extract_blk(p);
					break;
				case RUN: case BLOCR:
					proc_extract_pf(p);
					break;
				default:
					break;
			}
			switch_proc(pid);
		}
		kill_proc(SIGKILL * 0x100);
		return 1;

	} else if (!contain_id(p->p_shnd.ign, sigid)
			&& contain_id(SIG_DFLCNT, sigid)) {
		// Un signal CONT relance le processus même si un handler
		// est installé
		if (p->p_stat == STOP) {
			p->p_stat = RUN;
			sched_add_proc(pid);
		}
	}
	p->p_spnd |= ((sigset_t)1) << sigid;
	if (p->p_stat == BLOCK)
		proc_unblock_1(pid, p);
	return 0;
}

int sys_kill(pid_t pid, int signum) {
	if (pid < 0 || pid >= NPROC || signum < 0 || signum > SIG_COUNT) {
		set_errno(EINVAL);
		return -1;
	}
	proc_t* d = state.st_proc + pid;
	if (!proc_alive(d)) {
		set_errno(ESRCH);
		return -1;
	}
	
	if (!signum)       return 0;
	if (d->p_ring < 3) {
		set_errno(EPERM);
		return -1;
	}

	bool self = state.st_curr_pid == pid;
	int8_t  r = send_sig_to_proc(pid, signum - 1);
	if (!r || !self) return  0;
	
	schedule_proc();
}
