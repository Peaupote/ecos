#include <libc/string.h>

#include <kernel/param.h>
#include <kernel/kutil.h>
#include <kernel/file.h>
#include <kernel/proc.h>

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
    memcpy(fst[PROC_FS].fs_name, "tprc", 5);
    fst[PROC_FS].fs_mnt            = &fs_proc_mount;
    fst[PROC_FS].fs_lookup         = &fs_proc_lookup;
    fst[PROC_FS].fs_stat           = &fs_proc_stat;
    fst[PROC_FS].fs_read           = &fs_proc_read;
    fst[PROC_FS].fs_write          = &fs_proc_write;
    fst[PROC_FS].fs_touch          = &fs_proc_touch;
    fst[PROC_FS].fs_mkdir          = &fs_proc_mkdir;
    fst[PROC_FS].fs_getdents       = &fs_proc_getdents;
    fst[PROC_FS].fs_opench         = &fs_proc_opench;
    fst[PROC_FS].fs_open           = &fs_proc_open;
    fst[PROC_FS].fs_close          = &fs_proc_close;
    fst[PROC_FS].fs_rm             = &fs_proc_rm;
    fst[PROC_FS].fs_destroy_dirent = &fs_proc_destroy_dirent;
    fst[PROC_FS].fs_readsymlink    = &fs_proc_readsymlink;


    klogf(Log_info, "vfs", "setup ext2 file system");
    memcpy(fst[EXT2_FS].fs_name, "ext2", 5);
    fst[EXT2_FS].fs_mnt            = (fs_mnt_t*)&ext2_mount;
    fst[EXT2_FS].fs_lookup         = &ext2_lookup;
    fst[EXT2_FS].fs_stat           = (fs_stat_t*)&ext2_stat;
    fst[EXT2_FS].fs_read           = (fs_rdwr_t*)&ext2_read;
    fst[EXT2_FS].fs_write          = (fs_rdwr_t*)&ext2_write;
    fst[EXT2_FS].fs_touch          = (fs_create_t*)ext2_touch;
    fst[EXT2_FS].fs_mkdir          = 0; // not implemented yet
    fst[EXT2_FS].fs_getdents       = &ext2_getdents;
    fst[EXT2_FS].fs_opench         = &ext2_opench;
    fst[EXT2_FS].fs_open           = &ext2_open;
    fst[EXT2_FS].fs_close          = &ext2_close;
    fst[EXT2_FS].fs_rm             = 0; // not implemted yet
    fst[EXT2_FS].fs_destroy_dirent = 0;
    fst[EXT2_FS].fs_readsymlink    = 0;

    vfs_mount(PROC_MOUNT, PROC_FS, 0);
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
    fs_proc_pipe_path(path, pipeid, true); // out
    rt[0] = vfs_load(path, 0); // read
    fs_proc_pipe_path(path, pipeid, false);// in
    rt[1] = vfs_load(path, 0); // write
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

        if (!(ino = fs->fs_lookup(ino, name, info))) {
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

    v = state.st_files + free;
    memcpy(&v->vf_stat, &st, sizeof(struct stat));
    v->vf_stat.st_dev = dev->dev_id;
    v->vf_cnt = 1;

    fs->fs_open(st.st_ino, v, &dev->dev_info);

    return state.st_files + free;
}

void vfs_opench(vfile_t *vf, chann_adt_t* cdt) {
    struct device *dev = devices + vf->vf_stat.st_dev;
    struct fs *fs = fst + dev->dev_fs;
    return fs->fs_opench(vf->vf_stat.st_ino, cdt, &dev->dev_info);
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
        if (!vf->vf_cnt) {
            dev_t dev_id = vf->vf_stat.st_dev;
            struct device *dev = devices + dev_id;
            struct fs *fs = fst + dev->dev_fs;
            fs->fs_close(vf->vf_stat.st_ino, &dev->dev_info);
        }
    }
    return 0;
}

static vfile_t *
vfs_alloc(struct device *dev, const char *parent, const char *fname,
          mode_t perm, fs_create_t alloc) {
    struct stat st;
    struct fs *fs = fst + dev->dev_fs;
    char *path = (char*)(parent + strlen(dev->dev_mnt));
    kprintf("path %s\n", path);
    ino_t rc = vfs_lookup(&dev->dev_info, fs, path, &st);
    if (!rc) {
        klogf(Log_error, "vfs", "alloc: no file %s", path);
        return 0;
    }

    if (!(st.st_mode&TYPE_DIR)) {
        klogf(Log_error, "vfs", "alloc: %s is not a directory", parent);
        return 0;
    }

    size_t i;
    for (i = 0; i < NFILE; i++) {
        if (!state.st_files[i].vf_cnt) break;
    }

    if (i == NFILE) {
        klogf(Log_error, "vfs", "alloc: to much file open");
        return 0;
    }

    rc = alloc(rc, fname, perm, &dev->dev_info);
    if (!rc || fs->fs_stat(rc, &state.st_files[i].vf_stat, &dev->dev_info) < 0) {
        klogf(Log_error, "vfs", "failed to alloc %s / %s", parent, fname);
        return 0;
    }

    state.st_files[i].vf_stat.st_dev = dev->dev_id;
    state.st_files[i].vf_cnt = 1;
    return state.st_files + i;
}

vfile_t *vfs_create(const char *fname, mode_t perm) {
    struct device *dev = find_device(fname);
    if (!dev) {
        klogf(Log_info, "vfs", "no mount point %s", fname);
        return 0;
    }

    klogf(Log_error, "vfs", "create %s", fname);

    // extract parent name from full name
    char *filename = strrchr(fname, '/');
    if (!filename) {
        klogf(Log_error, "vfs", "invalid filename %s", fname);
        return 0;
    }

    char parent[256] = { 0 };
    memcpy(parent, fname, filename++ - fname);

    struct fs *fs = fst + dev->dev_fs;
    return vfs_alloc(dev, parent, filename, perm, fs->fs_touch);
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

int vfs_getdents(vfile_t *vf, struct dirent* dst, size_t sz,
                    chann_adt_t* cdt) {

    if (!(vf->vf_stat.st_mode & TYPE_DIR)) {
        klogf(Log_error, "vfs", "getdents: file %d is not a directory",
              vf->vf_stat.st_ino);
        vfs_close(vf);
        return -1;
    }

    klogf(Log_verb, "vfs", "getdents %d (device %d)",
          vf->vf_stat.st_ino, vf->vf_stat.st_dev);

    struct device *dev = devices + vf->vf_stat.st_dev;
    struct fs *fs = fst + dev->dev_fs;
    return fs->fs_getdents(vf->vf_stat.st_ino, dst, sz, cdt, &dev->dev_info);
}

int vfs_rm(const char *fname __attribute__((unused))) {
    kAssert(false);
    return 0;
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

    // check if dir is empty and save parent inode
    chann_adt_t cdt;
    char dbuf[512];
    int rc;
    fs->fs_opench(root, &cdt, &dev->dev_info);
    while((rc = fs->fs_getdents(root, (struct dirent*)dbuf,
                    512, &cdt, &dev->dev_info)) > 0) {
        struct dirent* de = (struct dirent*)dbuf;
        if (rc < de->d_rec_len) {
            klogf(Log_error, "rm",
                    "pas assez de place pour stocker le nom (1)");
            vfs_close(vf);
            return 0;
        }
        for (int i = 0; i < rc;
                i += de->d_rec_len, de = (struct dirent*)(dbuf + i)) {
            if (de->d_name_len == 2
                    && !strncmp(de->d_name, "..", 2))
                parent = de->d_ino;
            else if (de->d_ino &&
                !(de->d_name_len == 1 && de->d_name[0] == '.'))
                is_empty = 0;
        }
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

            fs->fs_opench(ino, &cdt, &dev->dev_info);
            while((rc = fs->fs_getdents(ino, (struct dirent*)dbuf,
                            512, &cdt, &dev->dev_info)) > 0) {
                struct dirent* de = (struct dirent*)dbuf;
                if (rc < de->d_rec_len) {
                    klogf(Log_error, "rm",
                            "pas assez de place pour stocker le nom (2)");
                    vfs_close(vf);
                    return 0;
                }
                for (int i = 0; i < rc; i += de->d_rec_len,
                        de = (struct dirent*)(dbuf + i)) {
                    if (de->d_ino != ino &&
                        !(de->d_name_len == 2
                            && !strncmp("..", de->d_name, 2))) {
                        PUSH(de->d_ino);
                    }
                    fs->fs_rm(de->d_ino, &dev->dev_info);
                }
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
