#ifndef _H_PROC2_FS
#define _H_PROC2_FS

/**
 * Not communicating with physical memory of any kind
 * gives informations about the kernel to the user through simple files
 *
 * The filsystem is architectured as follow:
 * /proc (ROOT_INO)
 *   - /proc/tty
 *   - /proc/pipes
 *   - /proc/pid
 *     - /proc/pid/stat process status
 *     - /proc/pid/cwd
 *     - /proc/pid/fd   files opened by process
 *
 */

#include <kernel/proc.h>
#include <kernel/file.h>
#include <headers/file.h>

#include <stdbool.h>

#define PROC_ROOT_INO 1
#define FP_PIPE_IN    1
#define FP_PIPE_OUT   2

#define PROC_MOUNT "/proc"

struct fp_pipe_cnt {
	char*   buf;
	size_t  sz;
	size_t  ofs;
};

struct fp_pipe {
	uint8_t open;
	mode_t  mode_in;
	mode_t  mode_out;
	struct fp_pipe_cnt cnt;
};

struct fp_pipe fp_pipes[NPIPE];
struct fp_pipe_cnt   ttyin_buf;
extern vfile_t* ttyin_vfile;


int      fs_proc_mount(void*, struct mount_info *info);
uint32_t fs_proc_lookup(const char *fname, ino_t parent, struct mount_info*);
int      fs_proc_stat(ino_t ino, struct stat *st, struct mount_info*);
int      fs_proc_read(ino_t ino, void*, off_t, size_t, struct mount_info*);
int      fs_proc_write(ino_t ino, void*, off_t, size_t, struct mount_info*);
struct dirent_it* fs_proc_opendir(ino_t, struct dirent_it*,
			char name_buf[], struct mount_info *);
struct dirent_it* fs_proc_readdir(struct dirent_it*, char name_buf[]);
ino_t    fs_proc_readsymlink(ino_t, char*, struct mount_info *);


uint32_t fs_proc_mkdir(ino_t, const char*, uint16_t, struct mount_info*);
ino_t    fs_proc_rm(ino_t ino, struct mount_info *);
ino_t    fs_proc_rmdir(ino_t ino, struct mount_info *);
ino_t    fs_proc_destroy_dirent(ino_t p, ino_t ino, struct mount_info*);
uint32_t fs_proc_touch(ino_t, const char*, uint16_t, struct mount_info*);


void     fs_proc_init_pipes();
uint32_t fs_proc_alloc_pipe(mode_t m_in, mode_t m_out);
static inline int fs_proc_pipe_path(char* dst,
			uint32_t pipeid, bool out) {
	return sprintf(dst, PROC_MOUNT "/pipes/%d%c", pipeid, out ? 'o' : 'i');
}
void     fs_proc_close_pipe(struct fp_pipe* p, uint8_t io);
size_t   fs_proc_write_pipe(struct fp_pipe_cnt* p,
							const void* src, size_t count);
size_t   fs_proc_read_pipe(struct fp_pipe_cnt* p, void* dst, size_t count);

void     fs_proc_init_tty_buf();
bool     fs_proc_std_to_tty(proc_t *);
static inline size_t fs_proc_write_tty(const char* src, size_t len) {
	size_t rt = fs_proc_write_pipe(&ttyin_buf, src, len);
	if (rt && ttyin_vfile) {
		ttyin_vfile->vf_stat.st_size = ttyin_buf.sz;
		vfs_unblock(ttyin_vfile);
	}
	return rt;
}

#endif
