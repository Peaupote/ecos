#include <libc/string.h>

#include <kernel/param.h>
#include <kernel/kutil.h>
#include <kernel/file.h>
#include <kernel/proc.h>
#include <kernel/sys.h>

#include <fs/ext2.h>
#include <fs/proc.h>

extern char root_partition[];
extern char root_partition_end[];

extern char home_partition[];
extern char home_partition_end[];

void vfs_init() {
    klogf(Log_info, "vfs", "Initialize");

    for (size_t i = 0; i < NDEV; i++) {
        devices[i].dev_id = i;
        devices[i].dev_free = 1;
    }

    for (size_t i = 0; i < NFILE; i++) {
        state.st_files[i].vf_cnt = 0;
        state.st_files[i].vf_waiting = PID_NONE;
    }

    klogf(Log_info, "vfs", "setup proc file system");
    memcpy(fst[PROC_FS].fs_name, "proc", 5);
    fst[PROC_FS].fs_mnt            = &fs_proc_mount;
    fst[PROC_FS].fs_lookup         = &fs_proc_lookup;
    fst[PROC_FS].fs_stat           = &fs_proc_stat;
    fst[PROC_FS].fs_read           = &fs_proc_read;
    fst[PROC_FS].fs_write          = &fs_proc_write;
    fst[PROC_FS].fs_touch          = &fs_proc_touch;
    fst[PROC_FS].fs_mkdir          = &fs_proc_mkdir;
    fst[PROC_FS].fs_truncate       = &fs_proc_truncate;
    fst[PROC_FS].fs_getdents       = &fs_proc_getdents;
    fst[PROC_FS].fs_opench         = &fs_proc_opench;
    fst[PROC_FS].fs_open           = &fs_proc_open;
    fst[PROC_FS].fs_close          = &fs_proc_close;
    fst[PROC_FS].fs_rm             = &fs_proc_rm;
    fst[PROC_FS].fs_destroy_dirent = &fs_proc_destroy_dirent;
    fst[PROC_FS].fs_link           = &fs_proc_link;
    fst[PROC_FS].fs_symlink        = &fs_proc_symlink;
    fst[PROC_FS].fs_readlink       = &fs_proc_readlink;

    klogf(Log_info, "vfs", "setup ext2 file system");
    memcpy(fst[EXT2_FS].fs_name, "ext2", 5);
    fst[EXT2_FS].fs_mnt            = (fs_mnt_t*)&ext2_mount;
    fst[EXT2_FS].fs_lookup         = &ext2_lookup;
    fst[EXT2_FS].fs_stat           = (fs_stat_t*)&ext2_stat;
    fst[EXT2_FS].fs_read           = (fs_rdwr_t*)&ext2_read;
    fst[EXT2_FS].fs_write          = (fs_rdwr_t*)&ext2_write;
    fst[EXT2_FS].fs_touch          = (fs_create_t*)ext2_touch;
    fst[EXT2_FS].fs_mkdir          = (fs_create_t*)&ext2_mkdir;
    fst[EXT2_FS].fs_truncate       = (fs_truncate_t*)&ext2_truncate;
    fst[EXT2_FS].fs_getdents       = &ext2_getdents;
    fst[EXT2_FS].fs_opench         = &ext2_opench;
    fst[EXT2_FS].fs_open           = &ext2_open;
    fst[EXT2_FS].fs_close          = &ext2_close;
    fst[EXT2_FS].fs_rm             = 0; // not implemted yet
    fst[EXT2_FS].fs_destroy_dirent = 0;
    fst[EXT2_FS].fs_link           = (fs_link_t*)&ext2_link;
    fst[EXT2_FS].fs_symlink        = (fs_symlink_t*)&ext2_symlink;
    fst[EXT2_FS].fs_readlink       = (fs_readlink_t*)&ext2_readlink;

    klogf(Log_info, "ext2", "root_part: %p - %p",
                    root_partition, root_partition_end);
    vfs_mount_root(EXT2_FS, root_partition);

    vfs_mount(PROC_MOUNT, PROC_FS, 0);

    klogf(Log_info, "ext2", "home_part: %p - %p",
                    home_partition, home_partition_end);
    vfs_mount("/home", EXT2_FS, home_partition);

}

bool vfs_find(const char* path1, const char* pathend1, dev_t* dev, ino_t* ino) {
    char buf[1024], *path = buf, *pathend;
    memcpy(path, path1, pathend1 - path1);
    pathend = path + (pathend1 - path1);
    *pathend = 0;

    // number of times we jumped back to begin
    // while following a symbolic link
    int cnt = 0;

begin:
    if (*path == '/') {
        *dev = ROOT_DEV;
        *ino = devices[ROOT_DEV].dev_info.root_ino;
        while(*path == '/') ++path;
    }

    struct device* dv = devices + *dev;
    struct fs*     fs = fst + dv->dev_fs;
    struct stat st;

    while(path < pathend) {
        char* end_part = strchrnul(path, '/');
        size_t len = end_part - path;
        if (len == 2 && path[0] == '.' && path[1] == '.'
            && *ino == dv->dev_info.root_ino) {
            *ino = dv->dev_parent_ino;
            *dev = dv->dev_parent_dev;
            dv   = devices + *dev;
            fs   = fst + dv->dev_fs;
        } else {
            if (len > 255) {
                klogf(Log_error, "vfs", "find: partie de chemin trop longue");
                return false;
            }
            char name[256];
            memcpy(name, path, len);
            name[len] = '\0';

            ino_t pino = *ino;
            *ino = fs->fs_lookup(pino, name, &dv->dev_info);
            if (!*ino) {
                klogf(Log_info, "vfs", "can't find %s", name);
                return false;
            }

            fs->fs_stat(*ino, &st, &dv->dev_info);
            if ((st.st_mode&0xf000) == TYPE_SYM) {
                int rc = fs->fs_readlink(*ino, name, 255, &dv->dev_info);
                kAssert(rc >= 0);
                name[rc] = 0;

                // if not last mem of path
                if (*end_part) {
                    if (++cnt > 10) { // probably trapped in a symlink loop
                        set_errno(ELOOP);
                        return false;
                    }

                    len = pathend - end_part;

                    // restart find at begining without recursion
                    // and on the concatenation of symlink path and remaining path
                    memmove(buf + rc, end_part, len);
                    memcpy(buf, name, rc);
                    buf[rc + len] = 0;

                    path = buf;
                    pathend = buf + rc + len;
                    *ino = pino;
                    goto begin;
                }
            }

            for (dev_t d = dv->dev_childs; ~d; d = devices[d].dev_sib)
                if (devices[d].dev_parent_red == *ino) {
                    *ino = devices[d].dev_info.root_ino;
                    *dev = d;
                    dv   = devices + *dev;
                    fs   = fst + dv->dev_fs;
                    break;
                }
        }

        path = end_part;
        while(*path == '/') ++path;
    }

    return true;
}

int vfs_mount_root(uint8_t fs, void *partition) {
    struct device* dv = devices + ROOT_DEV;

    if (!dv->dev_free || !fst[fs].fs_mnt(partition, &dv->dev_info)) {
        klogf(Log_info, "vfs", "failed to mount root");
        kpanic("Failed mounting");
    }

    dv->dev_fs         = fs;
    dv->dev_free       = 0;

    dv->dev_parent_dev = ROOT_DEV;
    dv->dev_parent_red = dv->dev_parent_ino = dv->dev_info.root_ino;

    dv->dev_childs     = ~(dev_t)0;
    dv->dev_sib        = ~(dev_t)0;

    klogf(Log_info, "vfs", "root sucessfully mounted");
    return ROOT_DEV;
}

int vfs_mount(const char *path, uint8_t m_fst, void *partition) {
    ino_t red_ino = devices[ROOT_DEV].dev_info.root_ino;
    dev_t red_dev = ROOT_DEV;

    if (!vfs_find(path, path + strlen(path), &red_dev, &red_ino)) {
        klogf(Log_error, "vfs", "%s not found", path);
        kpanic("Failed mounting");
    }

    struct fs* p_fs = fst + devices[red_dev].dev_fs;
    ino_t par_ino = p_fs->fs_lookup(red_ino, "..", &devices[red_dev].dev_info);
    kAssert(par_ino != 0);

    size_t d;
    for (d = 0; d < NDEV && !devices[d].dev_free; d++);

    struct device* dv = devices + d;
    if (d == NDEV || !fst[m_fst].fs_mnt(partition, &dv->dev_info)) {
        klogf(Log_error, "vfs", "failed to mount %s", path);
        kpanic("Failed mounting");
    }

    dv->dev_fs         = m_fst;
    dv->dev_free       = 0;

    dv->dev_parent_dev = red_dev;
    dv->dev_parent_red = red_ino;
    dv->dev_parent_ino = par_ino;

    dv->dev_childs     = ~(dev_t)0;
    dv->dev_sib        = devices[red_dev].dev_childs;
    devices[red_dev].dev_childs = d;

    klogf(Log_info, "vfs", "%s sucessfully mounted (device %d)", path, d);
    return d;
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

static vfile_t *vfs_load_cnt(const char *filename, int flags, int load_cnt) {
    if (!filename || !*filename) return NULL;
    if (++load_cnt > 10) { // probably in a symlink loop
        set_errno(ELOOP);
        return 0;
    }

    ino_t ino    = cur_proc()->p_cino;
    dev_t dev_id = cur_proc()->p_dev;

    if (!vfs_find(filename, filename + strlen(filename), &dev_id, &ino)) {
        set_errno(ENOENT);
        klogf(Log_info, "vfs", "file '%s' dont exists", filename);
        return 0;
    }

    struct device *dev = devices + dev_id;
    struct fs      *fs = fst + dev->dev_fs;
    struct stat st;
    fs->fs_stat(ino, &st, &dev->dev_info);

    if ((st.st_mode&0xf000) == TYPE_SYM && !(flags&O_NOFOLLOW)) {
        char buf[256] = { 0 };
        fs->fs_readlink(st.st_ino, buf, 255, &dev->dev_info);
        return vfs_load_cnt(buf, flags, load_cnt);
    }


    vfile_t *v;
    size_t free = NFILE;
    for (size_t i = 0; i < NFILE; i++) {
        v = state.st_files + i;
        if (v->vf_cnt) {
            if (v->vf_stat.st_ino == ino && v->vf_stat.st_dev == dev_id) {
                v->vf_cnt++;
                klogf(Log_info, "vfs", "file %s is open %d times",
                      filename, state.st_files[i].vf_cnt);
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

vfile_t *vfs_load(const char *filename, int flags) {
    return vfs_load_cnt(filename, flags, 0);
}

void vfs_opench(vfile_t *vf, chann_adt_t* cdt) {
    struct device *dev = devices + vf->vf_stat.st_dev;
    struct fs *fs = fst + dev->dev_fs;
    return fs->fs_opench(vf->vf_stat.st_ino, cdt, &dev->dev_info);
}

int vfs_absolute_path(ino_t ino, dev_t dev_id, char *dst, size_t len) {
    ino_t root_root_dev = devices[ROOT_DEV].dev_info.root_ino;
    ino_t pino;

    size_t pos = len;
    struct dirent dir;

    struct device *dev;
    struct fs *fs;

    dev = devices + dev_id;
    fs  = fst + dev->dev_fs;

    while (dev_id != ROOT_DEV || ino != root_root_dev) {
        pino = fs->fs_lookup(ino, "..", &dev->dev_info);

        // root of filesystem
        // jump to parent filesystem
        if (pino == ino) {
            dev_id = dev->dev_parent_dev;
            pino = dev->dev_parent_ino;
            ino = dev->dev_parent_red;
            dev = devices + dev_id;
            fs  = fst + dev->dev_fs;
            continue;
        }

        int rc, off = 0;
        while ((rc = fs->fs_read(pino, &dir, off, DIRENT_OFF,
                                &dev->dev_info)) > 0) {
            if (dir.d_ino == ino) {
                off += DIRENT_OFF;
                break;
            }

            off += dir.d_rec_len;
        }

        kAssert(dir.d_ino == ino);

        pos -= dir.d_name_len + 1;
        dst[pos + dir.d_name_len] = '/';
        fs->fs_read(pino, dst + pos, off, dir.d_name_len, &dev->dev_info);

        ino = pino;
    }

    dst[--pos] = '/';

    if (pos > 0) memmove(dst, dst + pos, len - pos);
    return len - pos;
}

int vfs_read(vfile_t *vfile, void *buf, off_t pos, size_t len) {
    dev_t dev_id = vfile->vf_stat.st_dev;
    struct device *dev = devices + dev_id;
    struct fs *fs = fst + dev->dev_fs;

    klogf(Log_verb, "vfs", "read %d (dev %d) at pos %d",
          vfile->vf_stat.st_ino, vfile->vf_stat.st_dev, pos);

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

ino_t vfs_create(const char *fullname, mode_t perm) {
    ino_t par_ino = cur_proc()->p_cino;
    dev_t par_dev = cur_proc()->p_dev;

    klogf(Log_info, "vfs", "create %s", fullname);

    // extract parent name from full name
    const char *parent_end = strrchr(fullname, '/');
    const char *filename   = parent_end + 1;
    if (!parent_end)
        filename = fullname;
    else if (!vfs_find(fullname, parent_end, &par_dev, &par_ino)) {
        set_errno(ENOENT);
        klogf(Log_error, "vfs", "can't find parent of %s", fullname);
        return 0;
    }

    struct device *dev = devices + par_dev;
    struct fs      *fs = fst + dev->dev_fs;
    struct stat     st;
    fs->fs_stat(par_ino, &st, &dev->dev_info);

    if (!(st.st_mode & TYPE_DIR)) {
        klogf(Log_error, "vfs", "alloc: %s parent is not a directory",
                fullname);
        return 0;

    }

    // create file
    fs_create_t *create = perm&TYPE_DIR ? fs->fs_mkdir : fs->fs_touch;
    return create(par_ino, filename, perm, &dev->dev_info);
}

ino_t vfs_truncate(vfile_t *vf) {
    struct device *dev = devices + vf->vf_stat.st_dev;
    struct fs *fs = fst + dev->dev_fs;
    ino_t ino = fs->fs_truncate(vf->vf_stat.st_ino, &dev->dev_info);
    if (ino) fs->fs_stat(ino, &vf->vf_stat, &dev->dev_info);
    return ino;
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


int vfs_link(const char *path1, const char *path2) {
    ino_t pino = cur_proc()->p_cino, tino = pino;
    dev_t pdev_id = cur_proc()->p_dev, tdev_id = pdev_id;

    if (!vfs_find(path1, path1 + strlen(path1), &tdev_id, &tino)) {
        set_errno(ENOENT);
        klogf(Log_info, "vfs", "file '%s' dont exists", path1);
        return -1;
    }

    const char *parent_end = strrchr(path2, '/');
    if (!parent_end) {
        parent_end = path2;
    } else if (!vfs_find(path2, parent_end, &pdev_id, &pino)) {
        set_errno(ENOENT);
        klogf(Log_info, "vfs", "file '%s' dont exists", path2);
        return -1;
    } else ++parent_end; // skip '/'

    if (pdev_id != tdev_id) {
        set_errno(EXDEV);
        klogf(Log_info, "vfs", "invalid cross device hard link");
        return -1;
    }

    struct device *dev = devices + pdev_id;
    struct fs *fs = fst + dev->dev_fs;

    return fs->fs_link(tino, pino, parent_end, &dev->dev_info);
}

int vfs_symlink(const char *path1, const char *path2) {
    ino_t pino = cur_proc()->p_cino;
    dev_t pdev_id = cur_proc()->p_dev;

    const char *parent_end = strrchr(path2, '/');
    if (!parent_end) {
        parent_end = path2;
    } else if (!vfs_find(path2, parent_end, &pdev_id, &pino)) {
        set_errno(ENOENT);
        klogf(Log_info, "vfs", "file '%s' dont exists", path2);
        return -1;
    } else ++parent_end; // skip '/'

    struct device *dev = devices + pdev_id;
    struct fs *fs = fst + dev->dev_fs;

    return fs->fs_symlink(pino, parent_end, path1, &dev->dev_info);
}

int vfs_readlink(const char *path, char *buf, size_t len) {
    ino_t ino = cur_proc()->p_cino;
    dev_t dev_id = cur_proc()->p_dev;

    if (!vfs_find(path, path + strlen(path), &dev_id, &ino)) {
        set_errno(ENOENT);
        return -1;
    }

    struct device *dev = devices + dev_id;
    struct fs *fs = fst + dev->dev_fs;

    int rc = fs->fs_readlink(ino, buf, len, &dev->dev_info);
    return rc;
}
