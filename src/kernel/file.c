#include <libc/string.h>

#include <kernel/kutil.h>
#include <kernel/file.h>
#include <kernel/proc.h>

#include <fs/pipe.h>
#include <fs/ext2.h>
#include <fs/dummy.h>
#include <fs/proc.h>

extern char home_partition[];
void vfs_init() {
    klogf(Log_info, "vfs", "Initialize");

    for (size_t i = 0; i < NDEV; i++) {
        devices[i].dev_id = i;
        devices[i].dev_mnt[0] = 0;
        devices[i].dev_free = 1;
    }

    for (size_t i = 0; i < NFILE; i++) {
        state.st_files[i].vf_cnt = 0;
    }

    klogf(Log_info, "vfs", "setup proc file system");
    memcpy(fst[PROC_FS].fs_name, "proc", 4);
    fst[PROC_FS].fs_mnt     = &proc_mount;
    fst[PROC_FS].fs_lookup  = &proc_lookup;
    fst[PROC_FS].fs_stat    = &proc_stat;
    fst[PROC_FS].fs_read    = &proc_read;
    fst[PROC_FS].fs_write   = &proc_write;
    fst[PROC_FS].fs_touch   = &proc_touch;
    fst[PROC_FS].fs_mkdir   = &proc_mkdir;
    fst[PROC_FS].fs_opendir = &proc_opendir;
    fst[PROC_FS].fs_readdir = &proc_readdir;

    klogf(Log_info, "vfs", "setup ext2 file system");
    memcpy(fst[EXT2_FS].fs_name, "ext2", 4);
    fst[EXT2_FS].fs_mnt     = (fs_mnt_t*)&ext2_mount;
    fst[EXT2_FS].fs_lookup  = (fs_lookup_t*)&ext2_lookup;
    fst[EXT2_FS].fs_stat    = (fs_stat_t*)&ext2_stat;
    fst[EXT2_FS].fs_read    = (fs_rdwr_t*)&ext2_read;
    fst[EXT2_FS].fs_write   = (fs_rdwr_t*)&ext2_write;
    fst[EXT2_FS].fs_touch   = 0; // not implemeted yet
    fst[EXT2_FS].fs_mkdir   = 0; // not implemented yet
    fst[EXT2_FS].fs_opendir = 0; // not implemented yet
    fst[EXT2_FS].fs_readdir = (fs_readdir_t*)&ext2_readdir;

    vfs_mount("/proc", PROC_FS, 0);
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

vfile_t *vfs_pipe() {
    size_t free = 0;
    for (size_t i = 0; i < NFILE; i++) {
        if (!free && !state.st_files[i].vf_cnt) free = i;
    }

    if (free == NFILE) {
        klogf(Log_info, "vfs", "can't find a free virtual file slot");
        return 0;
    }

    vfile_t *vf = state.st_files + free;
    struct pipe_inode *p = pipe_alloc();

    return vf;
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

static ino_t vfs_lookup(struct mount_info *info, struct fs *fs,
                        const char *full_name, struct stat *st) {
    char name[256] = { 0 };
    char *start, *end;

    if (!full_name) return 0;
    start = (char*)(*full_name == '/' ? full_name + 1 : full_name);
    end = index(start, '/');

    ino_t ino = info->root_ino;

    while (*start) {
        memcpy(name, start, end - start);
        name[end - start + 1] = 0;

        if (!(ino = fs->fs_lookup(name, ino, info)))
            return 0;

        if (!*end || !*(end+1)) break;
        start = end+1;
        end = index(start, '/');
    }

    return fs->fs_stat(ino, st, info);
}

vfile_t *vfs_load(const char *filename, uint32_t create) {
    struct device *dev = find_device(filename);
    if (!dev) {
        klogf(Log_info, "vfs", "no mount point %s", filename);
        return 0;
    }

    klogf(Log_info, "vfs", "load %s from %s (device id %d)",
          filename, dev->dev_mnt, dev->dev_id);

    struct fs *fs = &fst[dev->dev_fs];
    struct stat st;
    char *fname = (char*)(filename + strlen(dev->dev_mnt));
    ino_t rc = vfs_lookup(&dev->dev_info, fs, fname, &st);

    if (!rc) {
        if (!create || *index(fname, '/')) {
            klogf(Log_error, "vfs", "file %s dont exists", filename);
            return 0;
        }

        // TODO : create
        klogf(Log_error, "vfs", "create not implemeted yet");
        kAssert(false);
    }

    size_t free = 0;
    for (size_t i = 0; i < NFILE; i++) {
        if (state.st_files[i].vf_stat.st_ino == st.st_ino &&
            state.st_files[i].vf_stat.st_dev == dev->dev_id) {
            klogf(Log_info, "vfs", "file %s is already open", fname);
            return state.st_files + i;
        }

        if (!free && !state.st_files[i].vf_cnt) free = i;
    }

    if (free == NFILE) {
        klogf(Log_info, "vfs", "can't find a free virtual file slot");
        return 0;
    }

    memcpy(&state.st_files[free].vf_stat, &st, sizeof(struct stat));
    state.st_files[free].vf_stat.st_dev = dev->dev_id;
    state.st_files[free].vf_cnt = 0;
    return state.st_files + free;
}

int vfs_read(vfile_t *vfile, void *buf, off_t pos, size_t len) {
    struct device *dev = devices + vfile->vf_stat.st_dev;
    return  fst[dev->dev_fs].fs_read(vfile->vf_stat.st_ino,
                                    buf, pos, len, &dev->dev_info);
}

int vfs_write(vfile_t *vfile, void *buf, off_t pos, size_t len) {
    struct device *dev = devices + vfile->vf_stat.st_dev;
    return fst[dev->dev_fs].fs_write(vfile->vf_stat.st_ino,
                                    buf, pos, len, &dev->dev_info);
}

int vfs_close(vfile_t *vf) {
    if (vf) vf->vf_cnt = 0;
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

struct dirent *vfs_opendir(const char *fname) {
    vfile_t *vfile = vfs_load(fname, 0);
    struct dirent *dir = 0;

    if (!vfile || !(vfile->vf_stat.st_mode&TYPE_DIR)) goto ret;

    struct device *dev = devices + vfile->vf_stat.st_dev;
    struct fs *fs = fst + dev->dev_fs;
    dir = fs->fs_opendir(vfile->vf_stat.st_ino, &dev->dev_info);

ret:
    vfs_close(vfile);
    return dir;
}
