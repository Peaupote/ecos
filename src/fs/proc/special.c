#include "special.h"
#include <fs/proc.h>
#include <libc/string.h>
#include <kernel/file.h>
#include <kernel/proc.h>

int stat_read(pid_t pid, void *buf, off_t pos, size_t len) {
    proc_t *p = state.st_proc + pid;
    char file[256] = { 0 };

    char st = 'F';
    switch (p->p_stat) {
    case SLEEP: st = 'S'; break;
    case BLOCK: st = 'B'; break;
    case BLOCR: st = 'L'; break;
    case RUN:   st = 'R'; break;
    case ZOMB:  st = 'Z'; break;
    case FREE:
        st = 'F';
        kAssert(false);
        break;
    }

    int rc = sprintf(file, "%d (%s) %c", pid, p->p_cmd, st);
    if (!buf) return rc;

    char *dst = (char*)buf;
    size_t i;
    int ipos = (int)pos;

    for (i = 0; i < len && ipos < rc; i++) {
        *dst++ = file[ipos++];
    }

    return i;
}

int stat_stat(pid_t pid, ino_t ino, struct stat *st) {
    st->st_ino   = ino;
    st->st_mode  = TYPE_REG|400;
    st->st_nlink = 1;
    st->st_size  = stat_read(pid, 0, 0, 256);
    st->st_blksize = 1024;
    st->st_blocks = st->st_size >> 9;
    return ino;
}

int cmd_read(pid_t pid, void *buf, off_t pos, size_t len) {
    proc_t *p = state.st_proc + pid;
    size_t i = 0;
    off_t str_len = strlen(p->p_cmd);

    if (!buf) return str_len;

    char *dst = (char*)buf;
    char *src = p->p_cmd + pos;
    for (i = 0; i < len && pos < str_len; i++)
        *dst++ = src[pos++];

    return i;
}

int cmd_stat(pid_t pid, ino_t ino, struct stat *st) {
    st->st_ino   = ino;
    st->st_mode  = TYPE_REG|400;
    st->st_nlink = 1;
    st->st_uid   = 0;
    st->st_gid   = cmd_read(pid, 0, 0, 0);
    st->st_size  = 10; // change
    st->st_blksize = 1024;
    st->st_blocks = st->st_size >> 3;
    return ino;
}

ino_t alloc_special_file(pid_t pid, proc_op_t *read, proc_st_t*stat) {
    ino_t ino = proc_free_inode();
    struct proc_inode *inode = proc_inodes + ino;

    inode->in_block[SPEC_PID] = (uint64_t)pid;
    inode->in_block[SPEC_READ] = (uint64_t)read;
    inode->in_block[SPEC_STAT] = (uint64_t)stat;

    stat(pid, ino, &inode->st);

    return ino;
}
