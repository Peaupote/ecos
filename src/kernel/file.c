#include <kernel/kutil.h>
#include <kernel/file.h>
#include <kernel/proc.h>

#include <fs/dummy.h>

void vfs_init() {
    klogf(Log_info, "vfs", "Initialize");

    for (size_t i = 0; i < NDEV; i++) {
        devices[i].dev_id = i;
        devices[i].dev_mnt[0] = 0;
    }

    klogf(Log_info, "vfs", "setup dumb file system");
    memcpy(fst[DUMMY_FS].fs_name, "dumb", 4);
    fst[DUMMY_FS].fs_mnt     = dummy_mount;
    fst[DUMMY_FS].fs_load    = dummy_load;
    fst[DUMMY_FS].fs_create  = dummy_create;
    fst[DUMMY_FS].fs_read    = dummy_read;
    fst[DUMMY_FS].fs_write   = dummy_write;
    fst[DUMMY_FS].fs_readdir = dummy_readdir;
    fst[DUMMY_FS].fs_seek    = dummy_seek;

    void *spblk = fst[DUMMY_FS].fs_mnt();
    if (!spblk) {
        kpanic("Failed to mount /tmp");
    }

    memcpy(devices[0].dev_mnt, "/tmp", 5);
    devices[0].dev_fs = DUMMY_FS;
    devices[0].dev_spblk = spblk;

    klogf(Log_info, "vfs", "/tmp sucessfully mounted");
}

static struct device *find_device(char *fname) {
    // if mnt_pt is empty the mount point is unused
    // if more than one mount point is prefix of the file name
    // return the longest of the two prefix

    size_t i, mnt, len = 0;
    for (i = 0; i < NDEV; i++) {
        if (*devices[i].dev_mnt && !uis_prefix(fname, devices[i].dev_mnt)) {
            size_t l = ustrlen(devices[i].dev_mnt);
            if (l > len) {
                mnt = i;
                len = l;
            }
        }
    }

    return len > 0 ? devices + mnt : 0;
}

vfile_t *vfs_load(char *filename, uint32_t create) {
    struct device *dev = find_device(filename);
    if (!dev) {
        klogf(Log_info, "vfs", "no mount point %s", filename);
        return 0;
    }

    klogf(Log_info, "vfs", "load %s from %s (device id %d)",
          filename, dev->dev_mnt, dev->dev_id);

    struct fs *fs = &fst[dev->dev_fs];
    struct stat st;
    char *fname = filename + ustrlen(dev->dev_mnt);
    int rc = fs->fs_load(dev->dev_spblk, fname, &st, &fname);

    if (rc < 0) {
        if (!create || *uindex(fname, '/')) {
            klogf(Log_error, "vfs", "file %s dont exists", filename);
            return 0;
        }

        // TODO : fill stat
        rc = fs->fs_create(dev->dev_spblk, st.st_ino, fname);
        if (rc < 0) {
            return 0;
        }

        st.st_ino = rc;

        klogf(Log_info, "vfs", "create file %s inode %d", filename, rc);
    }

    size_t free = 0;
    for (size_t i = 0; i < NFILE; i++) {
        if (state.st_files[i].vf_stat.st_ino == st.st_ino) {
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

int vfs_seek(vfile_t *vfile, off_t pos) {
    struct device *dev = devices + vfile->vf_stat.st_dev;
    return fst[dev->dev_fs].fs_seek(dev->dev_spblk, vfile->vf_stat.st_ino, pos);
}

int vfs_read(vfile_t *vfile, void *buf, size_t len) {
    struct device *dev = devices + vfile->vf_stat.st_dev;
    return fst[dev->dev_fs].fs_read(dev->dev_spblk, vfile->vf_stat.st_ino,
                                   buf, len);
}

int vfs_write(vfile_t *vfile, void *buf, size_t len) {
    struct device *dev = devices + vfile->vf_stat.st_dev;
    return fst[dev->dev_fs].fs_write(dev->dev_spblk, vfile->vf_stat.st_ino,
                                    buf, len);
}
