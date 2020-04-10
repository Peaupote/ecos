#include <libc/string.h>

#include <kernel/param.h>
#include <kernel/kutil.h>
#include <kernel/file.h>
#include <kernel/proc.h>

#include <fs/pipe.h>
#include <fs/ext2.h>
#include <fs/proc.h>

extern char home_partition[];
extern char home_partition_end[];

void vfs_init() {
    klogf(Log_info, "vfs", "Initialize");

    for (size_t i = 0; i < NDEV; i++) {
        devices[i].dev_id = i;
        devices[i].dev_mnt[0] = 0;
        devices[i].dev_free = 1;
    }

    for (size_t i = 0; i < NFILE; i++) {
        state.st_files[i].vf_cnt = 0;
        state.st_files[i].vf_waiting = PID_NONE;
    }
	
	klogf(Log_info, "vfs", "setup proc file system");
    memcpy(fst[PROC2_FS].fs_name, "tprc", 5);
    fst[PROC2_FS].fs_mnt            = &fs_proc_mount;
    fst[PROC2_FS].fs_lookup         = &fs_proc_lookup;
    fst[PROC2_FS].fs_stat           = &fs_proc_stat;
    fst[PROC2_FS].fs_read           = &fs_proc_read;
    fst[PROC2_FS].fs_write          = &fs_proc_write;
    fst[PROC2_FS].fs_touch          = &fs_proc_touch;
    fst[PROC2_FS].fs_mkdir          = &fs_proc_mkdir;
    fst[PROC2_FS].fs_opendir        = &fs_proc_opendir;
    fst[PROC2_FS].fs_readdir        = &fs_proc_readdir;
    fst[PROC2_FS].fs_rm             = &fs_proc_rm;
    fst[PROC2_FS].fs_destroy_dirent = &fs_proc_destroy_dirent;
    fst[PROC2_FS].fs_readsymlink    = &fs_proc_readsymlink;


    klogf(Log_info, "vfs", "setup ext2 file system");
    memcpy(fst[EXT2_FS].fs_name, "ext2", 5);
    fst[EXT2_FS].fs_mnt            = (fs_mnt_t*)&ext2_mount;
    fst[EXT2_FS].fs_lookup         = (fs_lookup_t*)&ext2_lookup;
    fst[EXT2_FS].fs_stat           = (fs_stat_t*)&ext2_stat;
    fst[EXT2_FS].fs_read           = (fs_rdwr_t*)&ext2_read;
    fst[EXT2_FS].fs_write          = (fs_rdwr_t*)&ext2_write;
    fst[EXT2_FS].fs_touch          = (fs_create_t*)ext2_touch;
    fst[EXT2_FS].fs_mkdir          = 0; // not implemented yet
    fst[EXT2_FS].fs_opendir        = &ext2_opendir_it;
    fst[EXT2_FS].fs_readdir        = &ext2_readdir_it;
    fst[EXT2_FS].fs_rm             = 0; // not implemted yet
    fst[EXT2_FS].fs_destroy_dirent = 0;
    fst[EXT2_FS].fs_readsymlink    = 0;
    
    vfs_mount(PROC_MOUNT, PROC2_FS, 0);
	klogf(Log_info, "ext2", "home_part: %p - %p",
					home_partition, home_partition_end);
    vfs_mount("/home", EXT2_FS, home_partition);
}

int vfs_mount(const char *path, uint8_t fs, void *partition) {
    size_t i;
    for (i = 0; i < NDEV; i++) {
        if (devices[i].dev_free) break;
    }

    if (i == NDEV) {
        return 0;
    }

    if (!fst[fs].fs_mnt(partition, &devices[i].dev_info)) {
        klogf(Log_info, "vfs", "failed to mount %s", path);
        kpanic("Failed mounting");
    }

    strncpy(devices[i].dev_mnt, path, 256);
    devices[i].dev_fs = fs;
    devices[i].dev_free = 0;

    klogf(Log_info, "vfs", "%s sucessfully mounted (device %d)", path, i);
    return i;
}

uint32_t vfs_pipe(vfile_t* rt[2]) {
	uint32_t pipeid = fs_proc_alloc_pipe(TYPE_FIFO|0400, TYPE_FIFO|0200);
	if (!~pipeid) return ~(uint32_t)0;
	char path[256];
	fs_proc_pipe_path(path, pipeid, false);
	rt[0] = vfs_load(path, 0);
	fs_proc_pipe_path(path, pipeid, true);
	rt[1] = vfs_load(path, 0);
	kAssert(rt[0] && rt[1]);
	return pipeid;
}

struct device *find_device(const char *fname) {
    // if more than one mount point is prefix of the file name
    // return the longest of the two prefix

    size_t i, mnt, len = 0;
    for (i = 0; i < NDEV; i++) {
        if (!devices[i].dev_free && !is_prefix(fname, devices[i].dev_mnt)) {
            size_t l = strlen(devices[i].dev_mnt);
            if (l > len) {
                mnt = i;
                len = l;
            }
        }
    }

    return len > 0 ? devices + mnt : 0;
}

// TODO : follow symlink
static ino_t vfs_lookup(struct mount_info *info, struct fs *fs,
                        const char *full_name, struct stat *st) {
	klogf(Log_info, "vfs", "lookup for %s", full_name);
    char name[256] = { 0 };
    char *start, *end;

    if (!full_name) return 0;
    start = (char*)(*full_name == '/' ? full_name + 1 : full_name);
    end = strchrnul(start, '/');

    ino_t ino = info->root_ino;

    while (*start) {
        memcpy(name, start, end - start);
        name[end - start] = 0;

        if (!(ino = fs->fs_lookup(name, ino, info))) {
            return 0;
        }

        if (!*end || !*(end+1)) break;
        start = end+1;
        end = strchrnul(start, '/');
    }

    return fs->fs_stat(ino, st, info);
}

vfile_t *vfs_load(const char *filename, int flags) {
    if (!filename || !*filename) return 0;

    struct device *dev = find_device(filename);
    if (!dev) {
        klogf(Log_info, "vfs", "no mount point %s", filename);
        return 0;
    }

    klogf(Log_info, "vfs", "load %s from %s (device id %d)",
          filename, dev->dev_mnt, dev->dev_id);

    struct fs *fs = fst + dev->dev_fs;
    struct stat st;
    char *fname = (char*)(filename + strlen(dev->dev_mnt));
    ino_t rc = vfs_lookup(&dev->dev_info, fs, fname, &st);

    if (!rc) {
        klogf(Log_error, "vfs", "file %s dont exists", filename);
        return 0;
    }

    // TODO : follow symlink in lookup...
    if ((st.st_mode&0xf000) == TYPE_SYM) {
        if (flags&O_NOFOLLOW || flags&O_CREAT)
            return 0;

        char buf[256] = { 0 };
        fs->fs_readsymlink(st.st_ino, buf, &dev->dev_info);
        return vfs_load(buf, 0);
    }

    vfile_t *v;
    size_t free = NFILE;
    for (size_t i = 0; i < NFILE; i++) {
        v = state.st_files + i;
        if (v->vf_cnt) {
            if (v->vf_stat.st_ino == st.st_ino &&
                v->vf_stat.st_dev == dev->dev_id) {
                v->vf_cnt++;
                klogf(Log_info, "vfs", "file %s is open %d times",
                      fname, state.st_files[i].vf_cnt);
                return state.st_files + i;
            }

            continue;
        }

        if (i < free) free = i;
    }

    if (free == NFILE) {
        klogf(Log_info, "vfs", "can't find a free virtual file slot");
        return 0;
    }

    memcpy(&state.st_files[free].vf_stat, &st, sizeof(struct stat));
    state.st_files[free].vf_stat.st_dev = dev->dev_id;
    state.st_files[free].vf_cnt = 1;

    return state.st_files + free;
}

int vfs_read(vfile_t *vfile, void *buf, off_t pos, size_t len) {
    dev_t dev_id = vfile->vf_stat.st_dev;
    struct device *dev = devices + dev_id;
    struct fs *fs = fst + dev->dev_fs;

    klogf(Log_verb, "vfs", "read %d (dev %d)",
          vfile->vf_stat.st_ino, vfile->vf_stat.st_dev);

    int rc = fs->fs_read(vfile->vf_stat.st_ino, buf, pos, len, &dev->dev_info);
    fs->fs_stat(vfile->vf_stat.st_ino, &vfile->vf_stat, &dev->dev_info);
    vfile->vf_stat.st_dev = dev_id;
    return rc;
}

void vfs_unblock(vfile_t* vfile) {
	chann_t *c;
	cid_t cid;

	for (cid = vfile->vf_waiting; ~cid; ) {
		c = state.st_chann + cid;
		klogf(Log_info, "vfs", "write unblock cid %d", cid);
		proc_unblock_list(&c->chann_waiting);
		cid_t ncid   = c->chann_nxw;
		c->chann_nxw = cid;
		cid = ncid;
	}

	vfile->vf_waiting = ~0;
}

int vfs_write(vfile_t *vfile, void *buf, off_t pos, size_t len) {
    dev_t dev_id = vfile->vf_stat.st_dev;
    struct device *dev = devices + dev_id;
    struct fs *fs = fst + dev->dev_fs;

    int rc = fs->fs_write(vfile->vf_stat.st_ino, buf, pos, len, &dev->dev_info);
    fs->fs_stat(vfile->vf_stat.st_ino, &vfile->vf_stat, &dev->dev_info);
    vfile->vf_stat.st_dev = dev_id;

    // TODO : what happend for waiting ps if error ?
    if (rc > 0)
		vfs_unblock(vfile);

    return rc;
}

int vfs_destroy(vfile_t *vf) {
    klogf(Log_info, "vfs", "destroy vfile for inode %d (device %d)",
          vf->vf_stat.st_ino, vf->vf_stat.st_dev);

    // TODO tell channels
    vf->vf_cnt = 0;
    return 0;
}

int vfs_close(vfile_t *vf) {
    if (vf) {
        klogf(Log_info, "vfs", "close file %d (device %d)",
              vf->vf_stat.st_ino, vf->vf_stat.st_dev);
        --vf->vf_cnt;
        klogf(Log_info, "vfs", "still open %d times", vf->vf_cnt);
    }
    return 0;
}

static vfile_t *
vfs_alloc(struct device *dev, const char *parent, const char *fname,
          mode_t perm, fs_create_t alloc) {
    struct stat st;
    struct fs *fs = fst + dev->dev_fs;
    char *path = (char*)(parent + strlen(dev->dev_mnt));
    ino_t rc = vfs_lookup(&dev->dev_info, fs, path, &st);
    if (!rc) return 0;

    size_t i;
    for (i = 0; i < NFILE; i++) {
        if (!state.st_files[i].vf_cnt) break;
    }

    if (i == NFILE) return 0;

    rc = alloc(rc, fname, perm, &dev->dev_info);
    if (!rc || fs->fs_stat(rc, &state.st_files[i].vf_stat, &dev->dev_info) < 0)
        return 0;

    state.st_files[i].vf_stat.st_dev = dev->dev_id;
    state.st_files[i].vf_cnt = 1;
    return state.st_files + i;
}

vfile_t *vfs_touch(const char *parent, const char *fname, mode_t perm) {
    struct device *dev = find_device(parent);
    if (!dev) {
        klogf(Log_info, "vfs", "no mount point %s", parent);
        return 0;
    }

    klogf(Log_info, "vfs", "touch %s/%s", parent, fname);

    struct fs *fs = fst + dev->dev_fs;
    return vfs_alloc(dev, parent, fname, perm, fs->fs_touch);
}

vfile_t *vfs_mkdir(const char *parent, const char *fname, mode_t perm) {
    struct device *dev = find_device(parent);
    if (!dev) {
        klogf(Log_info, "vfs", "no mount point %s", parent);
        return 0;
    }

    klogf(Log_info, "vfs", "mkdir %s/%s", parent, fname);

    struct fs *fs = fst + dev->dev_fs;
    return vfs_alloc(dev, parent, fname, perm, fs->fs_mkdir);
}

struct dirent_it *vfs_opendir(vfile_t *vfile, struct dirent_it *dbuf,
		char* nbuf) {
    if (vfile) {
        if (!(vfile->vf_stat.st_mode&TYPE_DIR)) {
            klogf(Log_error, "vfs", "opendir: file %d is not a directory",
                  vfile->vf_stat.st_ino);
            vfs_close(vfile);
            return 0;
        }

        klogf(Log_info, "vfs", "opendir %d (device %d)",
              vfile->vf_stat.st_ino, vfile->vf_stat.st_dev);

        struct device *dev = devices + vfile->vf_stat.st_dev;
        struct fs *fs = fst + dev->dev_fs;
        return fs->fs_opendir(vfile->vf_stat.st_ino, dbuf, nbuf,
								&dev->dev_info);
    }
	return dbuf;
}

struct dirent_it *vfs_readdir(vfile_t *vfile, struct dirent_it *dir,
		char* nbuf) {
    struct device *dev = devices + vfile->vf_stat.st_dev;
    struct fs *fs = fst + dev->dev_fs;
    return fs->fs_readdir(dir, nbuf);
}

ino_t vfs_rmdir(const char *fname, uint32_t rec) {
    vfile_t *vf = vfs_load(fname, 0);
    if (!vf) return 0;
    if (!(vf->vf_stat.st_mode&TYPE_DIR)) return 0;

    dev_t did = vf->vf_stat.st_dev;
    struct device *dev = devices + did;
    struct fs *fs = fst + dev->dev_fs;

    // remember all inodes to update vfiles
    uint64_t inoset[1024] = { 0 };
    ino_t stack[1024], *top = stack;

#define SET(ino)   inoset[ino >> 6] |= 1 << (ino&(64-1));
#define ISSET(ino) (inoset[ino >> 6] & (1 << (ino&(64-1))))
#define PUSH(ino)  *top++ = ino
#define POP()      *--top
#define EMPTY()    (stack == top)

    ino_t root = vf->vf_stat.st_ino, ino;
    struct stat st;
    uint32_t is_empty = 1, parent = 0;
    struct dirent_it dbuf[1];
    struct dirent_it *dir;
	char nbuf[256];
    size_t size;

    // check if dir is empty and save parent inode
    dir = fs->fs_opendir(root, dbuf, nbuf, &dev->dev_info);
    for (size_t size = 0; size < vf->vf_stat.st_size;
         size += dir->cnt.d_rec_len, dir = fs->fs_readdir(dir, nbuf)) {
        if (dir->cnt.d_name_len == 2 && !strncmp(dir->cnt.d_name, "..", 2))
            parent = dir->cnt.d_ino;
        else if (dir->cnt.d_ino &&
                 !(dir->cnt.d_name_len == 1 && dir->cnt.d_name[0] == '.'))
            is_empty = 0;
    }

    kAssert(parent > 0);

    if (!rec) {
        if (!is_empty) {
            vfs_close(vf);
            return 0;
        }

        SET(root);
        goto destroy_root;
    }

    PUSH(root);
    while (!EMPTY()) {
        ino = POP();
        SET(ino);
        klogf(Log_info, "vfs", "remove inode %d", ino);

        fs->fs_stat(ino, &st, &dev->dev_info);
        if (st.st_mode&TYPE_DIR) {
            dir = fs->fs_opendir(ino, dbuf, nbuf, &dev->dev_info);
            for (size = 0; size < st.st_size;
                 size += dir->cnt.d_rec_len, dir = fs->fs_readdir(dir, nbuf)) {
                if (dir->cnt.d_ino != ino &&
                    !(dir->cnt.d_name_len == 2 
						&& !strncmp("..", dir->cnt.d_name, 2))) {
                    PUSH(dir->cnt.d_ino);
                }
                fs->fs_rm(dir->cnt.d_ino, &dev->dev_info);
            }
        }
    }

destroy_root:
    fs->fs_destroy_dirent(parent, root, &dev->dev_info);
    vfs_close(vf);

    SET(parent); // dont forget to update parent

    // update vfiles
    for (size_t i = 0; i < NFILE; i++) {
        if (state.st_files[i].vf_stat.st_dev == did &&
            ISSET(state.st_files[i].vf_stat.st_ino)) {
            fs->fs_stat(state.st_files[i].vf_stat.st_ino,
                       &state.st_files[i].vf_stat,
                       &dev->dev_info);
            state.st_files[i].vf_stat.st_dev = dev->dev_id;

            if (state.st_files[i].vf_stat.st_nlink == 0) {
                vfs_destroy(state.st_files + i);
            }
        }
    }

    return root;
}
