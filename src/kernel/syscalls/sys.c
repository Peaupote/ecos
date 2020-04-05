#include <kernel/sys.h>

#include <stdint.h>

#include <libc/string.h>
#include <kernel/int.h>
#include <kernel/proc.h>
#include <kernel/kutil.h>
#include <kernel/memory/shared_pages.h>
#include <util/elf64.h>
#include <util/misc.h>
#include <fs/proc.h>

/**
 * Syscalls
 * TODO: check args domain
 */

void sys_exit(int status) {
    klogf(Log_info, "syscall", "exit pid %d with status %d",
          state.st_curr_pid, status);

	kill_proc_nr(status);
}

pid_t sys_getpid() {
    klogf(Log_verb, "syscall", "getpid %d", state.st_curr_pid);
    return state.st_curr_pid;
}

pid_t sys_getppid() {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    klogf(Log_verb, "syscall", "getppid %d", p->p_ppid);
    return p->p_ppid;
}

static inline void rem_child_Z0(proc_t* p, proc_t* cp) {
    if (~cp->p_nxzb) {
        p->p_fchd = cp->p_nxzb;
        state.st_proc[cp->p_nxzb].p_prsb = PID_NONE;
        state.st_proc[cp->p_nxzb].p_nxsb = p->p_nxsb;
        if (~cp->p_nxsb)
            state.st_proc[cp->p_nxsb].p_prsb = cp->p_nxzb;
    } else {
        p->p_fchd = cp->p_nxsb;
        if (~cp->p_nxsb)
            state.st_proc[cp->p_nxsb].p_prsb = PID_NONE;
    }
}

pid_t sys_wait(int* rt_st) {
    klogf(Log_verb, "syscall", "wait");
    proc_t *p = &state.st_proc[state.st_curr_pid];
    if (~p->p_fchd) {
        // Si il y a un zombie alors le premier enfant en est un
        proc_t* cp = state.st_proc + p->p_fchd;
        if (cp->p_stat == ZOMB) {

            pid_t cpid = p->p_fchd;
            if (rt_st)
                *rt_st = cp->p_reg.b.rdi;

            rem_child_Z0(p, cp);

            --p->p_nchd;
            free_pid(cpid);
            return cpid;

        } else {
			proc_self_block(p);
            klogf(Log_info, "syscall", "process %d wait %d childs",
                    state.st_curr_pid, p->p_nchd);
            schedule_proc();
            never_reached return 0;
        }

    } else {
        klogf(Log_verb, "syscall",
              "process %d has no child. dont wait", state.st_curr_pid);
        return -1;
    }
}

pid_t sys_waitpid(pid_t cpid, int* rt_st) {
    if (cpid == PID_NONE) return sys_wait(rt_st);

    pid_t mpid = state.st_curr_pid;
    proc_t  *p = state.st_proc + mpid;
    proc_t *cp = state.st_proc + cpid;
    if (cp->p_ppid == mpid) {

        if (cp->p_stat == ZOMB) {

            if (rt_st) *rt_st = cp->p_reg.b.rdi;

            if (~cp->p_prsb) {
                state.st_proc[cp->p_prsb].p_nxzb = cp->p_nxzb;
                if (~cp->p_nxzb)
                    state.st_proc[cp->p_nxzb].p_prsb = cp->p_prsb;
            } else
                rem_child_Z0(p, cp);

            --p->p_nchd;
            free_pid(cpid);
            return cpid;
        }

		proc_self_block(p);
        klogf(Log_info, "syscall", "process %d wait child %d",
				mpid, cpid);
        schedule_proc();
        never_reached return 0;

    } else {
        klogf(Log_info, "syscall", "process %d is not %d's child",
                cpid, mpid);
        return -1;
    }
}

pid_t sys_fork() {
    proc_t *fp, *pp = state.st_proc + state.st_curr_pid;

    pid_t fpid = find_new_pid();
    // we didn't find place for a new processus
    if (!~fpid) return -1;

    fp          = state.st_proc + fpid;
    fp->p_ppid  = state.st_curr_pid;

    if (~pp->p_fchd) {
        proc_t *fc = state.st_proc + pp->p_fchd;
        if (fc->p_stat == ZOMB) {
            fp->p_prsb = pp->p_fchd;
            fp->p_nxsb = fc->p_nxsb;
            if (~fc->p_nxsb)
                state.st_proc[fc->p_nxsb].p_prsb = fpid;
            fc->p_nxsb = fpid;
        } else {
            fc->p_prsb = fpid;
            fp->p_nxsb = pp->p_fchd;
            fp->p_prsb = PID_NONE;
            pp->p_fchd = fpid;
        }
    } else {
        fp->p_nxsb = PID_NONE;
        fp->p_prsb = PID_NONE;
        pp->p_fchd = fpid;
    }

    fp->p_fchd  = PID_NONE;
    fp->p_nchd  = 0;
    fp->p_stat  = RUN;
    fp->p_ring  = pp->p_ring;
    fp->p_prio  = pp->p_prio;
	fp->p_brk   = pp->p_brk;
	fp->p_spnd  = 0;
	fp->p_shnd  = pp->p_shnd;

    pp->p_nchd++; // one more child
    memcpy(&fp->p_reg, &pp->p_reg, sizeof(struct reg));

    fp->p_pml4 = kmem_alloc_page();
    kmem_fork_paging(fp->p_pml4);
    paging_refresh();

    // copy file descriptors
    for (int i = 0; i < NFD; i++) {
        cid_t cid = pp->p_fds[i];
        fp->p_fds[i] = cid;
        if (cid >= 0 && cid < NCHAN &&
            state.st_chann[cid].chann_mode != UNUSED) {
            // one more process is referencing the channel
            state.st_chann[cid].chann_acc++;
        }
    }

    proc_create(fpid);
    fp->p_reg.b.rax = 0;

    sched_add_proc(fpid);

    klogf(Log_info, "syscall", "fork %d into %d",
            state.st_curr_pid, fpid);

    return fpid; // On retourne au parent
}

int sys_open(const char *fname, enum chann_mode mode) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    cid_t cid;
    for (cid = 0; cid < NCHAN; cid++)
        if (state.st_chann[cid].chann_mode == UNUSED) break;

    if (cid == NCHAN) {
        klogf(Log_error, "sys", "no channel available");
        return -1;
    }

    chann_t *c = state.st_chann + cid;
    c->chann_vfile = vfs_load(fname);
    if (!c->chann_vfile) {
        klogf(Log_error, "sys", "process %d couldn't open %s",
                state.st_curr_pid, fname);
        return -1;
    }

    for (int fd = 0; fd < NFD; fd++) {
        if (p->p_fds[fd] == -1) {
            c->chann_acc  = 1;
            c->chann_mode = mode;
            c->chann_pos  = 0;
            p->p_fds[fd] = cid;
            klogf(Log_info, "syscall", "process %d open %s on %d (cid %d)",
                  state.st_curr_pid, fname, fd, cid);
            return fd;
        }
    }

    return -1;
}

int sys_close(int filedes) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    // invalid file descriptor
    if (filedes < 0 || filedes > NFD)
        return -1;

    // file descriptor reference no channel
    if (p->p_fds[filedes] == -1)
        return -1;

    klogf(Log_info, "syscall", "close filedes %d (cid %d)",
          filedes, p->p_fds[filedes]);

    chann_t *c = &state.st_chann[p->p_fds[filedes]];
    if (c->chann_mode != UNUSED && --c->chann_acc == 0) {
        vfs_close(c->chann_vfile);
        c->chann_mode = UNUSED;
    }

    return 0;
}

int sys_dup(int fd) {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    int i;

    if (fd < 0 || fd > NFD || p->p_fds[fd] == -1)
        return -1;

    for (i = 0; i < NFD && p->p_fds[i] != -1; i++);

    if (i == NFD)
        // no file free descriptor
        return -1;

    p->p_fds[i] = p->p_fds[fd];
    return i;
}

int sys_pipe(int fd[2]) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    if (!fd) return -1;

    int fdin, fdout;

    for (fdin = 0; fdin < NFD && p->p_fds[fdin] != -1; fdin++);
    if (fd[0] == NFD) return -1;

    for (fdout = fdin; fdout < NFD && p->p_fds[fdout] != -1; fdout++);
    if (fd[0] == NFD) return -1;

    cid_t in, out;
    for (in = 0; in < NCHAN && state.st_chann[in].chann_mode != UNUSED; in++);
    if (in == NCHAN) return -1;

    for (out = in; out < NCHAN && state.st_chann[out].chann_mode != UNUSED; out++);
    if (out == NCHAN) return -1;

    chann_t *cin = &state.st_chann[in],
        *cout = &state.st_chann[out];

    fd[fdin] = in;
    fd[fdout] = out;

    cin->chann_acc   = 1;
    cin->chann_mode  = READ;
    cout->chann_acc  = 1;
    cout->chann_mode = WRITE;

    kAssert(false);// TODO : finish

    return 0;
}

ssize_t sys_read(int fd, uint8_t *d, size_t len) {
    proc_t *p  = &state.st_proc[state.st_curr_pid];;

    if (!d || fd < 0 || fd > NFD || p->p_fds[fd] == -1)
        return -1;

    chann_t *chann = &state.st_chann[p->p_fds[fd]];
    klogf(Log_info, "syscall", "process %d read %d on %d (cid %d)",
          state.st_curr_pid, len, fd, p->p_fds[fd]);

    vfile_t *vfile = chann->chann_vfile;
    int rc;

    if (chann->chann_mode != READ && chann->chann_mode != RDWR)
        return -1;

    switch (vfile->vf_stat.st_mode&0xf000) {
    case TYPE_DIR:
    case TYPE_REG:
        if (chann->chann_pos == vfile->vf_stat.st_size) // EOF
            return 0;

        rc = vfs_read(vfile, d, chann->chann_pos, len);
        if (rc > 0) chann->chann_pos += rc;
        klogf(Log_verb, "syscall", "%d char readed (pos %d)",
              rc, chann->chann_pos);
        return rc;

    case TYPE_CHAR:
    case TYPE_FIFO:
        if (vfile->vf_stat.st_size == 0) {
            wait_file(state.st_curr_pid, vfile);
        }

        return vfs_read(vfile, d, 0, len);
        break;

    default:
        return -1;
    }
}

ssize_t sys_write(int fd, uint8_t *s, size_t len) {
    proc_t *p  = &state.st_proc[state.st_curr_pid];;

    if (!s || fd < 0 || fd > NFD || p->p_fds[fd] == -1)
        return -1;

    chann_t *chann = &state.st_chann[p->p_fds[fd]];
    vfile_t *vfile = chann->chann_vfile;
    klogf(Log_verb, "syscall", "process %d write on %d",
            state.st_curr_pid, fd);

    size_t c = 0;
    int rc = 0;
    if (chann->chann_mode != WRITE && chann->chann_mode != RDWR) return -1;

    switch (vfile->vf_stat.st_mode&0xf000) {
    case TYPE_REG:
        rc = vfs_write(vfile, s, chann->chann_pos, len);
        if (rc > 0) chann->chann_pos += rc;
        return rc;

    case TYPE_FIFO:
        return vfs_write(vfile, s, 0, len);

    case TYPE_CHAR:
        // TODO : more efficient
        for (size_t i = 0; i < len; i++)
            kprintf("%c", *s++);

        return c;

    default:
        p->p_reg.b.rax = -1;
        return -1;
    }
}

off_t sys_lseek(int fd, off_t off) {
    proc_t *p = &state.st_proc[state.st_curr_pid];

    if (fd < 0 || fd > NFD) {
        klogf(Log_error, "syscall",
                "process %d lseek on invalid file descriptor %d",
                state.st_curr_pid, fd);
        return -1;
    }

    klogf(Log_verb, "syscall", "process %d lseek %d at %d",
            state.st_curr_pid, fd, off);

    state.st_chann[p->p_fds[fd]].chann_pos = off;

    return off;
}

int sys_setpriority(int prio) {
    proc_t *p = state.st_proc + state.st_curr_pid;
    if (prio > p->p_prio) return -1;
    p->p_prio = prio;
    return 0;
}
int sys_getpriority() {
    return state.st_proc[state.st_curr_pid].p_prio;
}

int sys_fstat(int fd, struct stat *st) {
    proc_t *p = state.st_proc + state.st_curr_pid;

    if (fd < 0 || fd > NFD) return -1;
    if (p->p_fds[fd] == -1) return -1;

    chann_t *c = state.st_chann + p->p_fds[fd];
    if (c->chann_mode == UNUSED) return -1;

    memcpy(st, &c->chann_vfile->vf_stat, sizeof (struct stat));
    return 0;
}

void* sys_sbrk(intptr_t inc) {//TODO: protect
    proc_t *p = state.st_proc + state.st_curr_pid;
	uint_ptr nbrk = (uint_ptr)( ((intptr_t)p->p_brk) + inc );

	kmem_paging_alloc_rng(align_to(p->p_brk, PAGE_SIZE), nbrk,
			PAGING_FLAG_W | PAGING_FLAG_U,
			PAGING_FLAG_W | PAGING_FLAG_U);
	
	for (uint_ptr a = align_to(nbrk, PAGE_SIZE); a < p->p_brk;
			a += PAGE_SIZE) {
		uint64_t *e = paging_page_entry(a);
		kAssert(*e & PAGING_FLAG_P);
		kmem_free_page(*e & PAGE_MASK);
		*e &= ~(uint64_t)PAGING_FLAG_P;
	}
	void *rt = (void*) p->p_brk;
	p->p_brk = nbrk;
	return rt;
}

int sys_debug_block(int v) {
    if (~v) return v;

    //On bloque le processus actuel
	proc_t* p = cur_proc();
	proc_self_block(p);
    schedule_proc();
    never_reached return -1;
}

uint64_t invalid_syscall() {
    proc_t *p = &state.st_proc[state.st_curr_pid];
    klogf(Log_error, "syscall", "invalid syscall code %d", p->p_reg.b.rax);
    return ~(uint64_t)0;
}

// Appels système de privilège ring 1

void prires_proc_struct(uint16_t new_ring) {
    state.st_proc[state.st_curr_pid].p_ring = rei_cast(uint8_t, new_ring);
}
