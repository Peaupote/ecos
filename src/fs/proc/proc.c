#include <stdbool.h>

#include <headers/unistd.h>

#include <kernel/file.h>
#include <kernel/tty.h>
#include <kernel/proc.h>

#include <fs/proc.h>
#include <fs/pipe.h>

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
#define PROC_PIPEI_INO(I) ((1<<31)|(I));
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

typedef struct {
	ino_t   ino;
	union {
		pid_t pid;
		unsigned num;
		uint32_t pipei;
	};
} dirent_dt_t;

typedef struct {
	pid_t pid;
} fpd_dt_t;

typedef bool (*dir_next)(fpd_dt_t*, bool, dirent_dt_t*, struct dirent*, char*);

struct fs_proc_dir {
	dir_next next;
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

static inline void dirent_stc(struct dirent* c, ino_t i, mode_t ty,
		char* name_buf, const char* name, size_t name_len) {
	c->d_ino       = i;
	c->d_rec_len   = 1;
	c->d_name_len  = name_len;
	c->d_file_type = ty >> FTYPE_OFS;
	c->d_name      = name_buf;
	memcpy(name_buf, name, name_len);
}

// -- root --

static bool root_next(fpd_dt_t* n __attribute__((unused)),
		bool begin, dirent_dt_t* dt,
		struct dirent* rt, char name_buf[]) {
	if (begin) {
		dt->pid = 0;
		dirent_stc(rt, PROC_TTY_INO, TYPE_DIR, name_buf, "tty", 3);
		return true;
	}
	pid_t i = dt->pid;
	if (i == 0) {
		dirent_stc(rt, PROC_PIPES_INO, TYPE_DIR, name_buf, "pipes", 5);
		dt->pid = 1;
		return true;
	}
	for (; i < NPROC; ++i) {
		if (proc_alive(state.st_proc + i)) {
			rt->d_ino       = i << PROC_INO_PID_SHIFT;
			rt->d_rec_len   = 1;
			rt->d_name_len  = sprintf(name_buf, "%d", i);
			rt->d_file_type = TYPE_DIR >> FTYPE_OFS;
			rt->d_name      = name_buf;
			dt->pid = i + 1;
			return true;
		}
	}
	return false;
}

// -- /tty --

static bool tty_next(fpd_dt_t* n __attribute__((unused)),
		bool begin, dirent_dt_t* dt,
		struct dirent* rt, char name_buf[]) {
	if (begin) dt->num = 0;
	if (dt->num > 2) return false;
	rt->d_ino       = PROC_TTYI_INO(dt->num);
	rt->d_rec_len   = 1;
	rt->d_name_len  = sprintf(name_buf, "tty%d", dt->num);
	rt->d_file_type = TYPE_CHAR >> FTYPE_OFS;
	rt->d_name      = name_buf;
	++dt->num;
	return true;
}

// -- /tty/<ttyi>
			
int ttyi_stat(file_ins* ins, struct stat* st) {
    st->st_mode = TYPE_CHAR | (ins->ttyi ? 0200 : 0400);
    st->st_nlink   = 1;
    st->st_size    = ins->ttyi ? 0 : ttyin_buf.sz;
    st->st_blksize = PROC_BLOCK_SIZE;
    st->st_blocks  = 0;
    return st->st_ino;
}
int ttyi_read(file_ins* ins, void* dst,
		off_t ofs __attribute__((unused)), size_t sz) {
	if (ins->ttyi) return -1;
	return fs_proc_read_pipe(&ttyin_buf, dst, sz);
}
int ttyi_write(file_ins* ins, void* src,
		off_t ofs __attribute__((unused)), size_t sz) {
	if (!ins->ttyi) return -1;
	tty_seq_t sq;
	tty_seq_init  (&sq);
	tty_seq_write (&sq, (const char*)src, sz);
	tty_seq_commit(&sq);
	return sz;
}

// -- /pipes --

static bool pipes_next(fpd_dt_t* n __attribute__((unused)),
		bool begin, dirent_dt_t* dt,
		struct dirent* rt, char name_buf[]) {

	for (uint32_t pi = begin ? 0 : dt->pipei; pi < 2 * NPIPE; ++pi) {
		if ((fp_pipes[pi >> 1].open >> (pi & 1)) & 1) {
			rt->d_ino       = PROC_PIPEI_INO(pi);
			rt->d_rec_len   = 1;
			rt->d_name_len  = sprintf(name_buf, "%d%c",
								pi>>1, (pi&1) ? 'o' : 'i');
			rt->d_file_type = TYPE_FIFO >> FTYPE_OFS;
			rt->d_name      = name_buf;
			dt->pipei = pi + 1;
			return true;
		}
	}
	return false;
}

// -- /pipes/<pipei>

static int pipei_stat(file_ins* ins, struct stat* st) {
    st->st_mode = TYPE_CHAR | ((ins->pipei&1) ? 0200 : 0400);
    st->st_nlink   = 1;
    st->st_size    = fp_pipes[ins->pipei >> 1].cnt.sz;
    st->st_blksize = PROC_BLOCK_SIZE;
    st->st_blocks  = 0;
    return st->st_ino;
}
static int pipei_read(file_ins* ins, void* dst,
		off_t ofs __attribute__((unused)), size_t sz) {
	if (ins->pipei & 1) return -1;
	return fs_proc_read_pipe(&fp_pipes[ins->pipei >> 1].cnt, dst, sz);
}
static int pipei_write(file_ins* ins, void* src,
		off_t ofs __attribute__((unused)), size_t sz) {
	if (!(ins->ttyi & 1)) return -1;
	return fs_proc_write_pipe(&fp_pipes[ins->pipei >> 1].cnt, src, sz);
}

// -- /<pid> --

static bool pid_next(fpd_dt_t* ddt,
		bool begin, dirent_dt_t* dt,
		struct dirent* rt, char name_buf[]) {
	if (begin){
		dirent_stc(rt, PROC_PID_FD_INO(ddt->pid), TYPE_DIR,
				name_buf, "fd", 2);
		dt->num = 0;
		return true;
	}
	switch (dt->num) {
		case 0:
			dirent_stc(rt, PROC_PID_STAT_INO(ddt->pid), TYPE_REG,
					name_buf, "stat", 4);
			dt->num = 1;
			return true;
		case 1:
			dirent_stc(rt, PROC_PID_CMD_INO(ddt->pid), TYPE_REG,
					name_buf, "cmd", 3);
			dt->num = 2;
			return true;
		default:
			return false;
	}
}

// -- /<pid>/fd --

static bool pid_fd_next(fpd_dt_t* ddt,
		bool begin, dirent_dt_t* dt,
		struct dirent* rt, char name_buf[]) {
	if (begin) dt->num = 0;
	proc_t* p = state.st_proc + ddt->pid;
	for(int fd = dt->num; fd < NFD; ++fd) {
		if (~p->p_fds[fd]) {
			rt->d_ino       = PROC_PID_FDI_INO(ddt->pid, fd);
			rt->d_rec_len   = 1;
			rt->d_name_len  = sprintf(name_buf, "%d", fd);
			rt->d_file_type = TYPE_SYM >> FTYPE_OFS;
			rt->d_name      = name_buf;
			dt->num = fd + 1;
			return true;
		}
	}
	return false;
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

static int dir_stat(file_ins* ins, struct stat* st) {
    st->st_mode    = TYPE_DIR|0400;
    st->st_nlink   = 1;

	int count = 0;
	{
		char nbuf[256];
		struct dirent it_cnt;
		dirent_dt_t   it_dt;
		it_dt.ino = st->st_ino;
		if (ins->d.next(&ins->d.dt, true, &it_dt, &it_cnt, nbuf)) {
			count = 1;
			while (ins->d.next(&ins->d.dt, false, &it_dt, &it_cnt, nbuf))
				++count;
		}
	}

    st->st_size    = count;
    st->st_blksize = PROC_BLOCK_SIZE;
    st->st_blocks  = 0;
    return st->st_ino;
}

static uint32_t dir_lookup(ino_t ino, file_ins* ins, const char* fname) {
	char nbuf[256];
	struct dirent it_cnt;
	dirent_dt_t   it_dt;
	it_dt.ino = ino;
	if (ins->d.next(&ins->d.dt, true, &it_dt, &it_cnt, nbuf)) {
		do {
			if (!strncmp(fname, it_cnt.d_name, it_cnt.d_name_len)
					&& fname[it_cnt.d_name_len] == '\0') {
				return it_cnt.d_ino;
			}
		} while (ins->d.next(&ins->d.dt, false, &it_dt, &it_cnt, nbuf));
	}
	return 0;
}
struct dirent_it* dir_opendir(file_ins* ins, struct dirent_it* dbuf,
		char* nbuf){
	dirent_dt_t* dt = (dirent_dt_t*)&dbuf->dt;
	return ins->d.next(&ins->d.dt, true, dt, &dbuf->cnt, nbuf)
			? dbuf : NULL;
				
}
struct dirent_it* dir_readdir(file_ins* ins, struct dirent_it* it,
		char* nbuf){
	dirent_dt_t* dt = (dirent_dt_t*)&it->dt;
	return ins->d.next(&ins->d.dt, false, dt, &it->cnt, nbuf)
			? it : NULL;
}

// -- redirection --

struct fs_proc_file {
	int      (*stat  )(file_ins*, struct stat*);
	int      (*read  )(file_ins*, void*, off_t, size_t);
	int      (*write )(file_ins*, void*, off_t, size_t);
	uint32_t (*lookup)(ino_t ino, file_ins*, const char* fname);
	struct dirent_it* (*open_dir)(file_ins*, struct dirent_it*, char*);
	struct dirent_it* (*read_dir)(file_ins*, struct dirent_it*, char*);
	ino_t    (*read_sym)(file_ins*, char*);
};

static struct fs_proc_file
	fun_dir = {
			.stat     = dir_stat,
			.read     = NULL,
			.write    = NULL,
			.lookup   = dir_lookup,
			.open_dir = dir_opendir,
			.read_dir = dir_readdir,
			.read_sym = NULL
		},
	fun_fdi = {
			.stat     = fdi_stat,
			.read     = fdi_read,
			.write    = NULL,
			.lookup   = NULL,
			.open_dir = NULL,
			.read_dir = NULL,
			.read_sym = fdi_read_sym
	},
	fun_cmd = {
			.stat     = reg_stat,
			.read     = cmd_read,
			.write    = NULL,
			.lookup   = NULL,
			.open_dir = NULL,
			.read_dir = NULL,
			.read_sym = NULL
	},
	fun_stat = {
			.stat     = reg_stat,
			.read     = stat_read,
			.write    = NULL,
			.lookup   = NULL,
			.open_dir = NULL,
			.read_dir = NULL,
			.read_sym = NULL
	},
	fun_ttyi = {
			.stat     = ttyi_stat,
			.read     = ttyi_read,
			.write    = ttyi_write,
			.lookup   = NULL,
			.open_dir = NULL,
			.read_dir = NULL,
			.read_sym = NULL
	},
	fun_pipei = {
			.stat     = pipei_stat,
			.read     = pipei_read,
			.write    = pipei_write,
			.lookup   = NULL,
			.open_dir = NULL,
			.read_dir = NULL,
			.read_sym = NULL
	};

static inline fpd_dt_t* set_dir_dt(file_ins* ins, dir_next dn) {
	ins->d.next = dn;
	return &ins->d.dt;
}

static bool fs_proc_from_ino(ino_t ino, file_ins* ins, struct fs_proc_file** rt) {
	klogf(Log_info, "fsproc", "access %d", ino);
	switch (ino) {
		case PROC_ROOT_INO:// /
			*rt = &fun_dir;
			set_dir_dt(ins, root_next);
			return true;
		case PROC_TTY_INO:// /tty/
			*rt = &fun_dir;
			set_dir_dt(ins, tty_next);
			return true;
		case PROC_PIPES_INO:// /pipes/
			*rt = &fun_dir;
			set_dir_dt(ins, pipes_next);
			return true;
		default:
			if (ino & (1<<31)) {
				// /pipes/<pipei>
				*rt = &fun_pipei;
				ins->pipei = ino & 0x7fffffff;
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
							set_dir_dt(ins, pid_next)->pid = pid;
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
							set_dir_dt(ins, pid_fd_next)->pid = pid;
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

struct dirent_it* fs_proc_opendir(ino_t ino, struct dirent_it* dbuf,
		char* nbuf, struct mount_info* info __attribute__((unused))) {
	file_ins ins;
	struct fs_proc_file* f;
	dirent_dt_t* dt = (dirent_dt_t*)&dbuf->dt;
	dt->ino = ino;
	
	if (fs_proc_from_ino(ino, &ins, &f) && f->open_dir)
		return f->open_dir(&ins, dbuf, nbuf);
	return NULL;
}
struct dirent_it* fs_proc_readdir(struct dirent_it* it, char* nbuf) {
	file_ins ins;
	struct fs_proc_file* f;
	dirent_dt_t* dt = (dirent_dt_t*)&it->dt;
	
	if (fs_proc_from_ino(dt->ino, &ins, &f) && f->read_dir)
		return f->read_dir(&ins, it, nbuf);
	return NULL;
}

#undef VAH
#undef GEN_RRT
