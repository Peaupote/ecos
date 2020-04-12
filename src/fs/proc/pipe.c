#include <fs/proc.h>

#include <kernel/memory/kmem.h>
#include <kernel/file.h>

#include <util/misc.h>

#include <headers/unistd.h>

#define PBSZ PAGE_SIZE

void fs_proc_init_pipes() {
	for (size_t i = 0; i < NPIPE; ++i) {
		fp_pipes[i].cnt.buf = NULL;
		fp_pipes[i].open = 0;
	}
}

uint32_t fs_proc_alloc_pipe(mode_t m_in, mode_t m_out) {
	for (size_t i = 0; i < NPIPE; ++i) {
		struct fp_pipe* p = fp_pipes + i;
		if (!p->open) {
			p->cnt.buf = (char*)kalloc_page();
			p->cnt.sz  = 0;
			p->cnt.ofs = 0;
			p->open    = 0b11;
			p->mode[fp_pipe_out] = m_out;
			p->mode[fp_pipe_in ] = m_in;
			p->vfs[fp_pipe_in] = p->vfs[fp_pipe_out] = NULL;
			return i;
		}
	}
	return ~(uint32_t)0;
}

void fs_proc_close_pipe(struct fp_pipe* p, uint8_t io) {
	kAssert(p->open);
	p->open &= ~io;
	if (!p->open) {
		kfree_page(p->cnt.buf);
		p->cnt.buf = NULL;
	}
}

size_t fs_proc_write_pipe(struct fp_pipe_cnt* p,
		const void* src, size_t count) {
	mina_size_t(&count, PBSZ - p->sz);
	const char* csrc = (const char*) src;
	const char* lim  = csrc + count;
	for(size_t ofs = p->ofs + p->sz; csrc != lim; ++ofs, ++csrc)
		p->buf[ofs % PBSZ] = *csrc;
	p->sz += count;
	return count;
}
size_t fs_proc_read_pipe(struct fp_pipe_cnt* p, void* dst, size_t count) {
	mina_size_t(&count, p->sz);
	char* cdst = (char*) dst;
	char* lim  = cdst + count;
	for(size_t ofs = p->ofs; cdst != lim; ++ofs, ++cdst)
		*cdst = p->buf[ofs % PBSZ];
	p->sz -= count;
	p->ofs = (p->ofs + count) % PBSZ;
	return count;
}

void fs_proc_init_tty_buf() {
	ttyin_buf.buf = (char*)kalloc_page();
	ttyin_buf.sz  = 0;
	ttyin_buf.ofs = 0;
}

static int open_channel(const char* path, int mflags) {
    cid_t cid = free_chann();
	if (cid == NCHAN) return ~0;
	chann_t* c = state.st_chann + cid;

    c->chann_vfile = vfs_load(path, 0);
	if (!c->chann_vfile) return ~0;

	c->chann_mode    = mflags;
	c->chann_waiting = PID_NONE;
	c->chann_nxw     = cid;
    c->chann_pos     = 0;
    c->chann_acc     = 1;
	return cid;
}

vfile_t* ttyin_vfile = NULL;
bool ttyin_force0 = false;

bool fs_proc_std_to_tty(proc_t* p) {
	p->p_fds[STDIN_FILENO ] = open_channel(PROC_MOUNT "/tty/tty0", READ);
	p->p_fds[STDOUT_FILENO] = open_channel(PROC_MOUNT "/tty/tty1", WRITE);
	p->p_fds[STDERR_FILENO] = open_channel(PROC_MOUNT "/tty/tty2", WRITE);
	
	return ~p->p_fds[STDIN_FILENO ] 
		&& ~p->p_fds[STDOUT_FILENO] 
		&& ~p->p_fds[STDERR_FILENO];
}
