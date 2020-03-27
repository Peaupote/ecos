#include <libc/string.h>

#include <kernel/kutil.h>
#include <kernel/file.h>
#include <kernel/proc.h>

#include <fs/ext2.h>
#include <fs/dummy.h>
#include <fs/proc.h>

extern char home_partition[];
void vfs_init() {
    klogf(Log_info, "vfs", "Initialize");

    for (size_t i = 0; i < NDEV; i++) {
        devices[i].dev_id = i;
        devices[i].dev_mnt[0] = 0;
    }

    /* klogf(Log_info, "vfs", "setup dumb file system"); */
    /* memcpy(fst[DUMMY_FS].fs_name, "dumb", 4); */
    /* fst[DUMMY_FS].fs_mnt     = &dummy_mount; */
    /* fst[DUMMY_FS].fs_load    = &dummy_load; */
    /* fst[DUMMY_FS].fs_create  = &dummy_create; */
    /* fst[DUMMY_FS].fs_read    = &dummy_read; */
    /* fst[DUMMY_FS].fs_write   = &dummy_write; */
    /* fst[DUMMY_FS].fs_readdir = &dummy_readdir; */
    /* fst[DUMMY_FS].fs_seek    = &dummy_seek; */

    /* klogf(Log_info, "vfs", "setup proc file system"); */
    /* memcpy(fst[PROC_FS].fs_name, "proc", 4); */
    /* fst[PROC_FS].fs_mnt     = &proc_mount; */
    /* fst[PROC_FS].fs_load    = &proc_load; */
    /* fst[PROC_FS].fs_create  = &proc_create; */
    /* fst[PROC_FS].fs_read    = &proc_read; */
    /* fst[PROC_FS].fs_write   = &proc_write; */
    /* fst[PROC_FS].fs_readdir = &proc_readdir; */
    /* fst[PROC_FS].fs_seek    = &proc_seek; */

    klogf(Log_info, "vfs", "setup ext2 file system");
    memcpy(fst[EXT2_FS].fs_name, "ext2", 4);
    fst[EXT2_FS].fs_mnt     = (fs_mnt_t*)&ext2_mount;
    fst[EXT2_FS].fs_lookup  = (fs_lookup_t*)&ext2_lookup;
    fst[EXT2_FS].fs_stat    = (fs_stat_t*)&ext2_stat;
    fst[EXT2_FS].fs_read    = (fs_rdwr_t*)&ext2_read;
    fst[EXT2_FS].fs_write   = (fs_rdwr_t*)&ext2_write;
    fst[EXT2_FS].fs_readdir = (fs_readdir_t*)&ext2_readdir;


    /* if (!fst[DUMMY_FS].fs_mnt(0, &devices[0].dev_info)) { */
    /*     kpanic("Failed to mount /tmp"); */
    /* } */

    /* memcpy(devices[0].dev_mnt, "/tmp", 6); */
    /* devices[0].dev_fs = DUMMY_FS; */

    /* klogf(Log_info, "vfs", "/tmp sucessfully mounted"); */

    /* memcpy(devices[1].dev_mnt, "/proc", 6); */
    /* devices[1].dev_fs = PROC_FS; */
    /* klogf(Log_info, "vfs", "/proc sucessfully mounted"); */

    memcpy(devices[2].dev_mnt, "/home", 6);
    devices[2].dev_fs = EXT2_FS;
    if (!fst[EXT2_FS].fs_mnt(home_partition, &devices[2].dev_info)) {
        kpanic("Failed to mount /home");
    }
    klogf(Log_info, "vfs", "/home sucessfully mounted");
}

static inline struct device *find_device(const char *fname) {
    // if mnt_pt is empty the mount point is unused
    // if more than one mount point is prefix of the file name
    // return the longest of the two prefix

    size_t i, mnt, len = 0;
    for (i = 0; i < NDEV; i++) {
        if (*devices[i].dev_mnt && !is_prefix(fname, devices[i].dev_mnt)) {
            size_t l = strlen(devices[i].dev_mnt);
            if (l > len) {
                mnt = i;
                len = l;
            }
        }
    }

    return len > 0 ? devices + mnt : 0;
}

static int vfs_lookup(struct mount_info *info, struct fs *fs,
                      const char *full_name, struct stat *st) {
    char name[256] = { 0 };
    char *start, *end;

    if (!full_name) return 0;
    start = (char*)(*full_name == '/' ? full_name + 1 : full_name);
    end = index(start, '/');

    uint32_t ino = info->root_ino;

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
    int rc = vfs_lookup(&dev->dev_info, fs, fname, &st);

    if (rc < 0) {
        if (!create || *index(fname, '/')) {
            klogf(Log_error, "vfs", "file %s dont exists", filename);
            return 0;
        }

        // TODO : create
        klogf(Log_error, "vfs", "create not implemeted yet");
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
