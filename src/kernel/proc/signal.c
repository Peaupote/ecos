#include <headers/signal.h>
#include <headers/sys.h>

#include <kernel/proc.h>

#include <util/misc.h>

void sys_sigsethnd(sighandler_t h) {
	cur_proc()->p_shnd.usr = h;
}

uint8_t sys_signal(int sigid, uint8_t hnd) {
	if (sigid < 0 || sigid >= SIG_COUNT
		|| ((((sigset_t)1) << sigid) & SIG_NOTCTB) )
			return ~(uint8_t)0;
	proc_t* p  = cur_proc();
	uint8_t rt = ((p->p_shnd.ign >> sigid) & 0x1)
			   | (((p->p_shnd.dfl >> sigid) & 0x1) << 1);
	p->p_shnd.ign = (p->p_shnd.ign & ~(((sigset_t)1) << sigid))
				  | (((sigset_t)(hnd & 0x1)) << sigid);
	p->p_shnd.dfl = (p->p_shnd.dfl & ~(((sigset_t)1) << sigid))
				  | (((sigset_t)((hnd>>1) & 0x1)) << sigid);
	return rt;
}

struct shnd_save { //sz = 18 * 8 = 0 [16]
	sigset_t blocked;
	struct base_reg b;
};

void sys_sigreturn() {
	proc_t *p = cur_proc();
	struct shnd_save* sv = (struct shnd_save*) p->p_reg.b.rsp;
	p->p_shnd.blk = sv->blocked | SIG_NOTCTB;
	p->p_reg.b = sv->b;
	iret_to_proc(p);
}

void proc_hndl_sig_i(int sigid) {
	klogf(Log_info, "hndl", "i%d -> %d", sigid, state.st_curr_pid);
	proc_t* p  = cur_proc();
	p->p_spnd &= ~(((sigset_t)1) << sigid);

	if ((p->p_shnd.ign >> sigid) & 1)
		return;
	if ((p->p_shnd.dfl >> sigid) & 1) {
		if ((SIG_DFLKIL >> sigid) & 1)
			kill_proc_nr((sigid+1) * 0x100);
		return; //ignore
	}

	// User handler
	if (p->p_stat == BLOCR) {
		// Si un appel système est en cours il est interrompu et renvoi -1
		p->p_stat      = RUN;
		p->p_reg.b.rax = ~(reg_t)0;
	}

	uint_ptr sv_loc = p->p_reg.b.rsp & ~(uint_ptr)0xf; //align 16
	sv_loc -= sizeof(struct shnd_save);
	struct shnd_save* sv = (struct shnd_save*) sv_loc;
	sv->b          = p->p_reg.b;
	sv->blocked    = p->p_shnd.blk;
	p->p_shnd.blk &= ~(((sigset_t) 1) << sigid);
	
	p->p_reg.b.rsp = sv_loc;// = 0 [16]
	rei_cast(int, p->p_reg.b.rdi) = sigid;
	p->p_reg.b.rip = (uint_ptr)p->p_shnd.usr;

	iret_to_proc(p);
}

bool send_sig_to_proc(pid_t pid, int sigid) {
	klogf(Log_info, "send", "send i%d to %d", sigid, (int)pid);
	proc_t* p  = state.st_proc + pid;
	if (sigid + 1 == SIGKILL) {
		if (state.st_curr_pid != pid) {
			switch (p->p_stat) {
				case BLOCK:
					if (p->p_reg.b.rax == SYS_EXECVE) {
						return true;//TODO
					}
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
		return true;
	}
	p->p_spnd |= ((sigset_t)1) << sigid;
	if (p->p_stat == BLOCK)
		proc_unblock_1(pid, p);
	return false;
}
