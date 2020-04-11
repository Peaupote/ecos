#include <stdbool.h>

#include <headers/unistd.h>

#include <kernel/file.h>
#include <kernel/tty.h>
#include <kernel/proc.h>

#include <fs/proc.h>

#include <libc/string.h>
#include <libc/stdio_strfuns.h>

#include <util/misc.h>

#define PROC_BLOCK_SIZE 1024

#define PROC_INO_PID_SHIFT   16
#define PROC_INO_PID_MASK    0x7fff0000
#define PROC_INO_PID_PART(P) (((ino_t)(P))<<PROC_INO_PID_SHIFT)

#define PROC_TTY_INO   2
#define PROC_PIPES_INO 3
#define PROC_TTYI_INO(I) (0x10|(I))
#define PROC_PIPEI_INO(I) ((1<<31)|(I))
#define PROC_PID_INO(P)      PROC_INO_PID_PART(P)
#define PROC_PID_STAT_INO(P) (PROC_INO_PID_PART(P)|1)
#define PROC_PID_CMD_INO(P)  (PROC_INO_PID_PART(P)|2)
#define PROC_PID_FD_INO(P)   (PROC_INO_PID_PART(P)|3)
#define PROC_PID_FDI_INO(P,F) (PROC_INO_PID_PART(P)|0x8000|(F))

struct spart_wtdt {
	size_t ofs;
	size_t count;
	char*  dst;
};

void spart_wt(void* pdt, const char* src, size_t c) {
	struct spart_wtdt* dt = (struct spart_wtdt*) pdt;
	if (dt->ofs) {
		if (dt->ofs >= c) {
			dt->ofs -= c;
			return;
		}
		src += dt->ofs;
		c   -= dt->ofs;
		dt->ofs = 0;
	}
	mina_size_t(&c, dt->count);
	memcpy(dt->dst, src, c);
	dt->count -= c;
	dt->dst   += c;
}
int spart_printf(void* dst, size_t ofs, size_t count,
		const char *fmt, ...) {
	struct spart_wtdt dt = {.ofs=ofs, .count=count, .dst=(char*)dst};
	va_list p;
	va_start(p, fmt);
	fpprintf(&spart_wt, &dt, fmt, p);
	va_end(p);
	return count - dt.count;
}

//

typedef struct {//sz <= CADT_SIZE
	uint8_t num0;
	union {
		pid_t pid;
		unsigned num;
		uint32_t pipei;
	};
} cdt_t;

typedef struct {
	pid_t pid;
} fpd_dt_t;

typedef int (*dir_getents)(fpd_dt_t*, int, 
						bool, cdt_t*, struct dirent*, size_t);

struct fs_proc_dir {
	ino_t parent;
	dir_getents ents;
	fpd_dt_t dt;
};

typedef union {
	struct fs_proc_dir d;
	struct {
		pid_t pid;
		int    fd;
	};
	uint8_t  ttyi;
	uint32_t pipei;
} file_ins;

static inline struct dirent* dirent_aux(int* rc, struct dirent* d,
		size_t* lsz, ino_t i, mode_t ty, size_t rlen) {
	if (*lsz < rlen) {
		if (!*rc && *lsz < offsetof(struct dirent, d_name)) {
			d->d_ino       = i;
			d->d_rec_len   = rlen;
			d->d_file_type = ty >> FTYPE_OFS;
			*rc = offsetof(struct dirent, d_name);
		}
		return NULL;
	}
	d->d_ino       = i;
	d->d_rec_len   = rlen;
	d->d_file_type = ty >> FTYPE_OFS;
	*rc  += rlen;
	*lsz -= rlen;
	return (struct dirent*)(rlen + (char*)d);
}

static inline bool dirent_stc(int* rc, struct dirent** d,
		size_t* lsz, ino_t i, mode_t ty,
		const char* name, size_t name_len) {
	size_t rlen = align_to_size(
					offsetof(struct dirent, d_name) + name_len + 1, 4);
	struct dirent* nx = dirent_aux(rc, *d, lsz, i, ty, rlen);
	if (nx)	{
		(*d)->d_name_len = name_len;
		memcpy((*d)->d_name, name, name_len + 1);
		*d = nx;
		return true;
	}
	return false;
}

// -- root --

static int root_getents(fpd_dt_t* n __attribute__((unused)),
		int rc, bool begin, cdt_t* dt, struct dirent* d, size_t sz) {
	pid_t i = begin ? -1 : dt->pid;
	switch (i) {
		case -1:
			if(!dirent_stc(&rc, &d, &sz, 
						PROC_TTY_INO, TYPE_DIR, "tty", 3)) {
				dt->pid = -1;
				return rc;
			}
			// FALLTHRU
		case 0:
			if (!dirent_stc(&rc, &d, &sz,
						PROC_PIPES_INO, TYPE_DIR, "pipes", 5)) {
				dt->pid = 0;
				return rc;
			}
			i = 1;
	}
	for (; i < NPROC; ++i) {
		if (proc_alive(state.st_proc + i)) {
			struct dirent* nx = dirent_aux(&rc, d, &sz,
					PROC_PID_INO(i), TYPE_DIR, 20);
			if (!nx) {
				dt->pid = i;
				return rc;
			}
			d->d_name_len = sprintf(d->d_name, "%d", i);
			d = nx;
		}
	}
	dt->pid = NPROC;
	return rc;
}

// -- /tty --

static int tty_getents(fpd_dt_t* n __attribute__((unused)),
		int rc, bool begin, cdt_t* dt, struct dirent* d, size_t sz) {
	if (begin) dt->num = 0;
	for (; dt->num <= 2; ++dt->num){
		struct dirent* nx = dirent_aux(&rc, d, &sz,
				PROC_TTYI_INO(dt->num), TYPE_CHAR, 16);
		if (!nx) return rc;
		d->d_name_len = sprintf(d->d_name, "tty%d", dt->num);
		d = nx;
	}
	return rc;
}

// -- /tty/<ttyi>
			
static int ttyi_stat(file_ins* ins, struct stat* st) {
    st->st_mode    = TYPE_CHAR | (ins->ttyi ? 0200 : 0400);
    st->st_nlink   = 1;
    st->st_size    = ins->ttyi ? 0 : ttyin_buf.sz;
    st->st_blksize = PROC_BLOCK_SIZE;
    st->st_blocks  = 0;
    return st->st_ino;
}
static int ttyi_read(file_ins* ins, void* dst,
		off_t ofs __attribute__((unused)), size_t sz) {
	if (ins->ttyi) return -1;
	return fs_proc_read_pipe(&ttyin_buf, dst, sz);
}
static int ttyi_write(file_ins* ins, void* src,
		off_t ofs __attribute__((unused)), size_t sz) {
	if (!ins->ttyi) return -1;
	tty_seq_t sq;
	tty_seq_init  (&sq);
	tty_seq_write (&sq, (const char*)src, sz);
	tty_seq_commit(&sq);
	return sz;
}

// -- /pipes --

static int pipes_getents(fpd_dt_t* n __attribute__((unused)),
		int rc, bool begin, cdt_t* dt, struct dirent* d, size_t sz) {

	for (uint32_t pi = begin ? 0 : dt->pipei; pi < 2 * NPIPE; ++pi) {
		if ((fp_pipes[pi >> 1].open >> (pi & 1)) & 1) {
			struct dirent* nx = dirent_aux(&rc, d, &sz,
					PROC_PIPEI_INO(pi), TYPE_FIFO, 20);
			if (!nx) {
				dt->pipei = pi;
				return rc;
			}
			d->d_name_len  = sprintf(d->d_name, "%d%c",
									pi>>1, (pi&1) ? 'o' : 'i');
			d = nx;
		}
	}
	dt->pipei = 2 * NPIPE;
	return rc;
}

// -- /pipes/<pipei>

static int pipei_stat(file_ins* ins, struct stat* st) {
    st->st_mode    = TYPE_FIFO | ((ins->pipei&1) ? 0200 : 0400);
    st->st_nlink   = 1;
    st->st_size    = fp_pipes[ins->pipei >> 1].cnt.sz;
    st->st_blksize = PROC_BLOCK_SIZE;
    st->st_blocks  = 0;
    return st->st_ino;
}
static int pipei_read(file_ins* ins, void* dst,
		off_t ofs __attribute__((unused)), size_t sz) {
	if (!(ins->pipei & 1)) return -1;
	struct fp_pipe* pi = fp_pipes + (ins->pipei >> 1);
	int rc =fs_proc_read_pipe(&pi->cnt, dst, sz);
	if (rc) {
		if (pi->vf_out)
			pi->vf_out->vf_stat.st_size = pi->cnt.sz;
		if (pi->vf_in)
			pi->vf_in->vf_stat.st_size  = pi->cnt.sz;
	}
	return rc;
}
static int pipei_write(file_ins* ins, void* src,
		off_t ofs __attribute__((unused)), size_t sz) {
	if (ins->pipei & 1) return -1;
	struct fp_pipe* pi = fp_pipes + (ins->pipei >> 1);
	int wc = fs_proc_write_pipe(&pi->cnt, src, sz);
	if (wc > 0) {
		if (pi->vf_out) {
			pi->vf_out->vf_stat.st_size = pi->cnt.sz;
			vfs_unblock(pi->vf_out);
		}
		if (pi->vf_in)
			pi->vf_in->vf_stat.st_size  = pi->cnt.sz;
	}
	return wc;
}

// -- /<pid> --

static int pid_getents(fpd_dt_t* ddt,
		int rc, bool begin, cdt_t* dt, struct dirent* d, size_t sz) {

	switch (begin ? 0 : dt->num) {
		case 0:
			if(!dirent_stc(&rc, &d, &sz, 
					PROC_PID_FD_INO(ddt->pid), TYPE_DIR, "fd", 2)) {
				dt->num = 0;
				return rc;
			}
			//FALLTHRU
		case 1:
			if(!dirent_stc(&rc, &d, &sz, 
					PROC_PID_STAT_INO(ddt->pid), TYPE_REG, "stat", 4)) {
				dt->num = 1;
				return rc;
			}
			//FALLTHRU
		case 2:
			if(!dirent_stc(&rc, &d, &sz, 
					PROC_PID_CMD_INO(ddt->pid), TYPE_REG, "cmd", 3)) {
				dt->num = 2;
				return rc;
			}
			//FALLTHRU
		default:
			dt->num = 3;
			return rc;
	}
}

// -- /<pid>/fd/ --

static int pid_fd_getents(fpd_dt_t* ddt,
		int rc, bool begin, cdt_t* dt, struct dirent* d, size_t sz) {

	
	proc_t* p = state.st_proc + ddt->pid;

	for(unsigned fd = begin ? 0 : dt->num; fd < NFD; ++fd) {
		if (~p->p_fds[fd]) {
			struct dirent* nx = dirent_aux(&rc, d, &sz,
					PROC_PID_FDI_INO(ddt->pid, fd), TYPE_SYM, 20);
			if (!nx) {
				dt->num = fd;
				return rc;
			}
			d->d_name_len  = sprintf(d->d_name, "%d", fd);
			d = nx;
		}
	}
	dt->num = NFD;
	return rc;
}

// -- /<pid>/fd/<fd> --

static int fdi_stat(file_ins* n __attribute__((unused)), struct stat* st) {
    st->st_mode    = TYPE_SYM;
    st->st_nlink   = 1;
    st->st_size    = 0;
    st->st_blksize = PROC_BLOCK_SIZE;
    st->st_blocks  = 0;
    return st->st_ino;
}
static int fdi_read(file_ins* ins __attribute__((unused)),
		void* d __attribute__((unused)),
		off_t ofs __attribute__((unused)),
		size_t sz __attribute__((unused))) {
	//TODO: obtenir le chemin du fichier
	return -1;
}
static ino_t fdi_read_sym(file_ins* ins __attribute__((unused)),
		char* d __attribute__((unused))) {
	return 0;
}

// -- /<pid>/cmd --

static int cmd_read(file_ins* ins, void* d, off_t ofs, size_t sz) {
	struct spart_wtdt wtdt = {.ofs=ofs, .count=sz, .dst=(char*)d};
	const char* cmd = state.st_proc[ins->pid].p_cmd;
	spart_wt(&wtdt, cmd, strlen(cmd));
	return sz - wtdt.count;
}

// -- /<pid>/stat --

static int stat_read(file_ins* ins, void* d, off_t ofs, size_t sz) {
	proc_t* p = state.st_proc + ins->pid;
	return spart_printf(d, ofs, sz, "%d (%s) %c",
			ins->pid, p->p_cmd, proc_state_char[p->p_stat]);
}

// -- régulier --

static int reg_stat(file_ins* n __attribute__((unused)), struct stat* st) {
    st->st_mode    = TYPE_REG|0400;
    st->st_nlink   = 1;
    st->st_size    = ~0;
    st->st_blksize = PROC_BLOCK_SIZE;
    st->st_blocks  = 0;
    return st->st_ino;
}

// -- dossiers --

static int dir_stat(file_ins* ins __attribute__((unused)),
		struct stat* st) {
    st->st_mode    = TYPE_DIR|0400;
    st->st_nlink   = 1;
    st->st_size    = 0;
    st->st_blksize = PROC_BLOCK_SIZE;
    st->st_blocks  = 0;
    return st->st_ino;
}

static uint32_t dir_lookup(ino_t ino, file_ins* ins, const char* fname) {
	if (!strcmp(fname, "."))
		return ino;
	if (!strcmp(fname, ".."))
		return ins->d.parent;
	char dbuf[512];
	cdt_t cdt;
	int rc = ins->d.ents(&ins->d.dt, 0, true, &cdt, (struct dirent*)dbuf, 512);
	if (rc > 0) {
		do {
			struct dirent *d = (struct dirent*)dbuf;
			kAssert(rc >= d->d_rec_len);

			for (int i = 0; i < rc;
					i += d->d_rec_len, d = (struct dirent*)(dbuf+i)) {
				if (!strncmp(fname, d->d_name, d->d_name_len)
						&& fname[d->d_name_len] == '\0') {
					return d->d_ino;
				}
			}

		} while ((rc = ins->d.ents(&ins->d.dt, 0, false, &cdt, 
					(struct dirent*)dbuf, 512)) > 0);
	}
	return 0;
}

static int dir_getdents(ino_t ino, file_ins* ins, struct dirent* d,
		size_t sz, chann_adt_t* p_cdt) {
	cdt_t* cdt = (cdt_t*)p_cdt;
	int rc = 0;
	switch (cdt->num0) {
		case 0:
			if(!dirent_stc(&rc, &d, &sz, ino, TYPE_DIR, ".", 1))
				return rc;
			//FALLTHRU
		case 1:
			if(!dirent_stc(&rc, &d, &sz, ins->d.parent, TYPE_DIR, "..", 2)) {
				cdt->num0 = 1;
				return rc;
			}
			//FALLTHRU
		case 2:
			cdt->num0 = 3;
			return ins->d.ents(&ins->d.dt, rc, true, cdt, d, sz);
		case 3:
			return ins->d.ents(&ins->d.dt, rc, false, cdt, d, sz);
		default:
			return -1;
	}
}

static void dir_opench(file_ins* ins __attribute__((unused)),
						chann_adt_t* p_cdt) {
	((cdt_t*)p_cdt)->num0 = 0;
}

// -- redirection --

struct fs_proc_file {
	void     (*opench)(file_ins*, chann_adt_t*);
	int      (*stat  )(file_ins*, struct stat*);
	int      (*read  )(file_ins*, void*, off_t, size_t);
	int      (*write )(file_ins*, void*, off_t, size_t);
	uint32_t (*lookup)(ino_t ino, file_ins*, const char* fname);
	int      (*getdents)(ino_t ino, file_ins*, struct dirent*, size_t,
							chann_adt_t*);
	ino_t    (*read_sym)(file_ins*, char*);
};

static struct fs_proc_file
	fun_dir = {
			.opench   = dir_opench,
			.stat     = dir_stat,
			.read     = NULL,
			.write    = NULL,
			.lookup   = dir_lookup,
			.getdents = dir_getdents,
			.read_sym = NULL
		},
	fun_fdi = {
			.opench   = NULL,
			.stat     = fdi_stat,
			.read     = fdi_read,
			.write    = NULL,
			.lookup   = NULL,
			.getdents = NULL,
			.read_sym = fdi_read_sym
	},
	fun_cmd = {
			.opench   = NULL,
			.stat     = reg_stat,
			.read     = cmd_read,
			.write    = NULL,
			.lookup   = NULL,
			.getdents = NULL,
			.read_sym = NULL
	},
	fun_stat = {
			.opench   = NULL,
			.stat     = reg_stat,
			.read     = stat_read,
			.write    = NULL,
			.lookup   = NULL,
			.getdents = NULL,
			.read_sym = NULL
	},
	fun_ttyi = {
			.opench   = NULL,
			.stat     = ttyi_stat,
			.read     = ttyi_read,
			.write    = ttyi_write,
			.lookup   = NULL,
			.getdents = NULL,
			.read_sym = NULL
	},
	fun_pipei = {
			.opench   = NULL,
			.stat     = pipei_stat,
			.read     = pipei_read,
			.write    = pipei_write,
			.lookup   = NULL,
			.getdents = NULL,
			.read_sym = NULL
	};

static inline fpd_dt_t* set_dir_dt(file_ins* ins, ino_t p, dir_getents e) {
	ins->d.parent = p;
	ins->d.ents   = e;
	return &ins->d.dt;
}

static bool fs_proc_from_ino(ino_t ino, file_ins* ins, struct fs_proc_file** rt) {
	klogf(Log_verb, "fsproc", "access %d = %x", (int)ino, (int)ino);
	switch (ino) {
		case PROC_ROOT_INO:// /
			*rt = &fun_dir;
			set_dir_dt(ins, PROC_ROOT_INO, root_getents);
			return true;
		case PROC_TTY_INO:// /tty/
			*rt = &fun_dir;
			set_dir_dt(ins, PROC_ROOT_INO, tty_getents);
			return true;
		case PROC_PIPES_INO:// /pipes/
			*rt = &fun_dir;
			set_dir_dt(ins, PROC_ROOT_INO, pipes_getents);
			return true;
		default:
			if (ino & (1<<31)) {
				// /pipes/<pipei>
				*rt = &fun_pipei;
				ins->pipei = ino & 0x7fffffff;
				klogf(Log_verb, "fsproc", "access pipe %d", (int)ins->pipei);
				return true;
			} else if (ino & (0x7fff << 16)) {
				pid_t pid = ino >> 16;
				if (pid >= NPROC || !proc_alive(state.st_proc + pid))
					return false;
				if (ino & (1<<15)) {// /<pid>/fd/<fd>
					int fd = ino & 0x7fff;
					if (fd >= NFD || !~state.st_proc[pid].p_fds[fd])
						return 0;
					*rt      = &fun_fdi;
					ins->pid = pid;
					ins->fd  = fd;
					return true;
				} else {
					switch (ino & 0x7fff) {
						case 0: // /<pid>/
							*rt = &fun_dir;
							set_dir_dt(ins, PROC_ROOT_INO, pid_getents)
								->pid = pid;
							return true;
						case 1: // /<pid>/stat
							*rt = &fun_stat;
							ins->pid = pid;
							return true;
						case 2: // /<pid>/cmd
							*rt = &fun_cmd;
							ins->pid = pid;
							return true;
						case 3: // /<pid>/fd/
							*rt = &fun_dir;
							set_dir_dt(ins, PROC_PID_INO(pid), pid_fd_getents)
								->pid = pid;
							return true;
						default:
							return false;
					}
				}
			} else if ((ino & 0x1fff0) == 0x10) {// /tty/tty<i>
				int i = ino & 0xf;
				if (i > 2) return false;
				*rt = &fun_ttyi;
				ins->ttyi = i;
    			return true;
			} else return false;
	}
}

int fs_proc_mount(void *partition __attribute__((unused)),
               struct mount_info *info) {
    // irrelevant here
    info->sp = 0;
    info->block_size = 1024;

    info->root_ino   = PROC_ROOT_INO;

	fs_proc_init_pipes();
	fs_proc_init_tty_buf();

    return 1;
}

uint32_t fs_proc_mkdir(ino_t n1 __attribute__((unused)),
		  const char*        n2 __attribute__((unused)),
		  uint16_t           n3 __attribute__((unused)),
		  struct mount_info* n4 __attribute__((unused))) {
	return 0;
}
ino_t fs_proc_rm(ino_t     n1 __attribute__((unused)),
		struct mount_info *n2 __attribute__((unused))) {
	return 0;
}
ino_t fs_proc_rmdir(ino_t    n1 __attribute__((unused)),
		  struct mount_info *n2 __attribute__((unused))) {
	return 0;
}
ino_t fs_proc_destroy_dirent(ino_t n1 __attribute__((unused)),
		        ino_t              n2 __attribute__((unused)),
			    struct mount_info* n3 __attribute__((unused))) {
	return 0;
}
uint32_t fs_proc_touch(ino_t n1 __attribute__((unused)),
		  const char*        n2 __attribute__((unused)),
		  uint16_t           n3 __attribute__((unused)),
		  struct mount_info* n4 __attribute__((unused))) {
	return 0;
}

#define GEN_RRT(R, N, M, E, AT, AN) \
	R N(ino_t ino, AT,\
			struct mount_info* none __attribute__((unused))) {\
		file_ins ins; \
		struct fs_proc_file* f; \
		if (fs_proc_from_ino(ino, &ins, &f) && f->M) \
			return f->M(&ins, AN); \
		return E; \
	}
#define VAH(...) __VA_ARGS__

void fs_proc_opench(ino_t ino, chann_adt_t* cdt,
		struct mount_info* info __attribute__((unused))) {
	file_ins ins;
	struct fs_proc_file* f;
	if (fs_proc_from_ino(ino, &ins, &f) && f->opench)
		f->opench(&ins, cdt);
}
uint32_t fs_proc_lookup(const char *fname, ino_t ino,
		struct mount_info* info __attribute__((unused))) {
	file_ins ins;
	struct fs_proc_file* f;
	if (fs_proc_from_ino(ino, &ins, &f) && f->lookup)
		return f->lookup(ino, &ins, fname);
	return 0;
}
int fs_proc_stat(ino_t ino, struct stat* st,
		struct mount_info* none __attribute__((unused))) {
	file_ins ins;
	struct fs_proc_file* f;
	if (fs_proc_from_ino(ino, &ins, &f) && f->stat) {
		st->st_ino = ino;
		return f->stat(&ins, st);
	}
	return -1;
}
GEN_RRT(int, fs_proc_read,  read,  -1, VAH(void* d, off_t o, size_t s), VAH(d,o,s))
GEN_RRT(int, fs_proc_write, write, -1, VAH(void* d, off_t o, size_t s), VAH(d,o,s))
GEN_RRT(ino_t, fs_proc_readsymlink, read_sym, 0, char* d, d)

int fs_proc_getdents(ino_t ino, struct dirent* d, size_t sz,
				chann_adt_t* cdt,
				struct mount_info* info __attribute__((unused))) {
	file_ins ins;
	struct fs_proc_file* f;
	if (fs_proc_from_ino(ino, &ins, &f) && f->getdents)
		return f->getdents(ino, &ins, d, sz, cdt);
	return -1;
}

#undef VAH
#undef GEN_RRT
