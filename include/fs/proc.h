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

#define PROC_MOUNT "/proc"

enum fp_pipe_end {
    fp_pipe_in  = 0, // Write end
    fp_pipe_out = 1  // Read end
};

struct fp_pipe_cnt {
    char*   buf;
    size_t  sz;
    size_t  ofs;
};

struct fp_pipe {
    uint8_t  open;
    mode_t   mode[2];
    vfile_t* vfs[2];
    struct fp_pipe_cnt cnt;
};

struct fp_pipe fp_pipes[NPIPE];

struct fp_pipe_cnt   ttyin_buf;
extern vfile_t*    ttyin_vfile;
extern bool       ttyin_force0;


int      fs_proc_mount(void*, struct mount_info *info);
uint32_t fs_proc_lookup(ino_t parent, const char *fname, struct mount_info*);
int      fs_proc_stat(ino_t ino, struct stat *st, struct mount_info*);
int      fs_proc_read(ino_t ino, void*, off_t, size_t, struct mount_info*);
int      fs_proc_write(ino_t ino, void*, off_t, size_t, struct mount_info*);

int      fs_proc_getdents(ino_t, struct dirent*, size_t,
                            chann_adt_t*, struct mount_info*);
void     fs_proc_opench(ino_t, chann_adt_t*, struct mount_info*);
void     fs_proc_open(ino_t, vfile_t*, struct mount_info*);
void     fs_proc_close(ino_t, struct mount_info*);

ino_t    fs_proc_readsymlink(ino_t, char*, struct mount_info *);


uint32_t fs_proc_mkdir(ino_t, const char*, uint16_t, struct mount_info*);
ino_t    fs_proc_rm(ino_t ino, struct mount_info *);
ino_t    fs_proc_rmdir(ino_t ino, struct mount_info *);
ino_t    fs_proc_destroy_dirent(ino_t p, ino_t ino, struct mount_info*);
uint32_t fs_proc_touch(ino_t, const char*, uint16_t, struct mount_info*);
ino_t    fs_proc_truncate(ino_t, struct mount_info*);


void     fs_proc_init_pipes();
uint32_t fs_proc_alloc_pipe(mode_t m_in, mode_t m_out);
static inline int fs_proc_pipe_path(char* dst,
            uint32_t pipeid, enum fp_pipe_end ed) {
    return sprintf(dst, PROC_MOUNT "/pipes/%d%c", pipeid, ed ? 'o' : 'i');
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
static inline void fs_proc_send0_tty() {
    ttyin_force0 = true;
    if (ttyin_vfile) {
        ttyin_vfile->vf_stat.st_size = 1;
        vfs_unblock(ttyin_vfile);
    }
}

#endif
