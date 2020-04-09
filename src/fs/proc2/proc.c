#include <stdbool.h>

#include <headers/unistd.h>

#include <kernel/file.h>
#include <kernel/proc.h>

#include <fs/proc2.h>
#include <fs/pipe.h>

#include <libc/string.h>

#define PROC_BLOCK_SIZE 1024

#define PROC_TTY_INO   2
#define PROC_PIPES_INO 3

#define PROC_INO_PID_SHIFT 16
#define PROC_INO_PID_MASK  0x7fff0000

typedef struct {
	ino_t   ino;
	union {
		pid_t pid;
	};
} dirent_dt_t;

typedef struct {
	pid_t pid;
} fpd_dt_t;

typedef bool (*dir_next)(fpd_dt_t*, bool, dirent_dt_t*, struct dirent*, char*);

struct fs_proc_dir {
	ino_t    ino;
	dir_next next;
	fpd_dt_t dt;
};

typedef union {
	struct fs_proc_dir d;
} file_ins;

static inline void dirent_dir(struct dirent* c, ino_t i,
		char* name_buf, const char* name, size_t name_len) {
	c->d_ino       = i;
	c->d_rec_len   = 1;
	c->d_name_len  = name_len;
	c->d_file_type = TYPE_DIR >> FTYPE_OFS;
	c->d_name      = name_buf;
	memcpy(name_buf, name, name_len);
}

// -- root --

static bool root_next(fpd_dt_t* n __attribute__((unused)),
		bool begin, dirent_dt_t* dt,
		struct dirent* rt, char name_buf[]) {
	if (begin) {
		dt->pid = 0;
		dirent_dir(rt, PROC_TTY_INO, name_buf, "tty", 3);
		return true;
	}
	pid_t i = dt->pid;
	if (i == 0) {
		dirent_dir(rt, PROC_PIPES_INO, name_buf, "pipes", 5);
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

// -- dossiers --

static int dir_stat(file_ins* ins, struct stat* st) {
    st->st_ino     = ins->d.ino;
    st->st_mode    = TYPE_DIR|0400;
    st->st_nlink   = 1;

	int count = 0;
	{
		char nbuf[256];
		struct dirent it_cnt;
		dirent_dt_t   it_dt;
		it_dt.ino = ins->d.ino;
		if (ins->d.next(&ins->d.dt, true, &it_dt, &it_cnt, nbuf)) {
			count = 1;
			while (ins->d.next(&ins->d.dt, false, &it_dt, &it_cnt, nbuf))
				++count;
		}
	}

    st->st_size    = count;
    st->st_blksize = PROC_BLOCK_SIZE;
    st->st_blocks  = 0;
    return ins->d.ino;
}

static uint32_t dir_lookup(file_ins* ins, const char* fname) {
	char nbuf[256];
	struct dirent it_cnt;
	dirent_dt_t   it_dt;
	it_dt.ino = ins->d.ino;
	if (!ins->d.next(&ins->d.dt, true, &it_dt, &it_cnt, nbuf)) {
		do {
			if (!strncmp(fname, it_cnt.d_name, it_cnt.d_name_len)
					&& fname[it_cnt.d_name_len] == '\0')
				return it_cnt.d_ino;
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
	uint32_t (*lookup)(file_ins*, const char* fname);
	struct dirent_it* (*open_dir)(file_ins*, struct dirent_it*, char*);
	struct dirent_it* (*read_dir)(file_ins*, struct dirent_it*, char*);
};

static struct fs_proc_file
	fun_dir = {
			.stat     = dir_stat,
			.read     = NULL,
			.write    = NULL,
			.lookup   = dir_lookup,
			.open_dir = dir_opendir,
			.read_dir = dir_readdir
		},
	fun_todo = {
			.stat     = NULL,
			.read     = NULL,
			.write    = NULL,
			.lookup   = NULL,
			.open_dir = NULL,
			.read_dir = NULL
		};

static inline fpd_dt_t* set_dir_dt(file_ins* ins, ino_t ino, dir_next dn) {
	ins->d.ino  = ino;
	ins->d.next = dn;
	return &ins->d.dt;
}

static mode_t fs_proc_from_ino(ino_t ino, file_ins* ins, struct fs_proc_file** rt) {
	switch (ino) {
		case PROC_ROOT_INO:// /
			*rt = &fun_dir;
			set_dir_dt(ins, ino, root_next);
			return TYPE_DIR|0400;
		case PROC_TTY_INO:// /tty/
			*rt = &fun_todo;
			return TYPE_DIR|0400;
		case PROC_PIPES_INO:// /pipes/
			*rt = &fun_todo;
			return TYPE_DIR|0400;
		default:
			if (ino & (1<<31)) {
				// /pipes/fileid
				*rt = &fun_todo;
				return 0;
			} else if (ino & (0x7fff << 16)) {
				pid_t pid = ino >> 16;
				if (pid >= NPROC || !proc_alive(state.st_proc + pid))
					return 0;
				if (ino & (1<<15)) {// /<pid>/fd/<fd>
					int fd = ino & 0x7fff;
					if (fd >= NFD || !~state.st_proc[pid].p_fds[fd])
						return 0;
					*rt = &fun_todo;
					return TYPE_SYM;
				} else {
					switch (ino & 0x7fff) {
						case 0: // /<pid>/
							*rt = &fun_todo;
							return TYPE_DIR|0600;
						case 1: // /<pid>/stat
							*rt = &fun_todo;
							return TYPE_REG;
						case 2: // /<pid>/cmd
							*rt = &fun_todo;
							return TYPE_REG;
						case 3: // /<pid>/fd/
							*rt = &fun_todo;
							return TYPE_REG|0600;
						default:
							return 0;
					}
				}
			} else if ((ino & 0x1fff0) == 0x10) {// /tty/tty<i>
				int i = ino & 0xf;
				if (i > 2) return 0;
				*rt = &fun_todo;
				return TYPE_CHAR|0400;
			} else return 0;
	}
}

int fs_proc_mount(void *partition __attribute__((unused)),
               struct mount_info *info) {
    // irrelevant here
    info->sp = 0;
    info->block_size = 1024;

    info->root_ino   = PROC_ROOT_INO;

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
		if (fs_proc_from_ino(ino, &ins, &f) && f->M)\
			return f->M(&ins, AN); \
		return E; \
	}
#define VAH(...) __VA_ARGS__

uint32_t fs_proc_lookup(const char *fname, ino_t ino,
		struct mount_info* info __attribute__((unused))) {
	file_ins ins;
	struct fs_proc_file* f;
	if (fs_proc_from_ino(ino, &ins, &f) && f->lookup)
		return f->lookup(&ins, fname);
	return 0;
}
GEN_RRT(int, fs_proc_stat,  stat,  -1, struct stat *st, st)
GEN_RRT(int, fs_proc_read,  read,  -1, VAH(void* d, off_t o, size_t s), VAH(d,o,s))
GEN_RRT(int, fs_proc_write, write, -1, VAH(void* d, off_t o, size_t s), VAH(d,o,s))

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

//TODO
ino_t fs_proc_readsymlink(ino_t ino, char *dst,
                       struct mount_info *info __attribute__((unused))) {
	file_ins ins;
	struct fs_proc_file* f;
	if ((fs_proc_from_ino(ino, &ins, &f) & TYPE_SYM) // TODO mask, == ?
			&& f->read) {
		f->read(&ins, dst, 0, ~(size_t)0); //TODO: size lim
	}
    return ino;
}
#undef VAH
#undef GEN_RRT
