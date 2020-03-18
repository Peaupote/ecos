#include <fs/proc.h>
#include <util/string.h>
#include <kernel/file.h>
#include <kernel/proc.h>

// no pid is larger than 5 digit in base 10
static int atoi(char *s, size_t len) {
    size_t i, res = 0;
    for (i = 0; *s && i < len; i++, s++) {
        if (*s < '0' || *s > '9') return -1;
        res = res * 10 + (*s - '0');
    }

    return res;
}

void *proc_mount() {
    // nothing to do because we access directly to kernel data
    return 0;
}

int proc_load(void *super, char *fname, struct stat *st, char **end) {
    st->st_ino = 0;

    if (*fname == '/') fname++;
    if (!*fname) return 0;

    klogf(Log_info, "procfs", "load %s", fname);
    *end = uindex(fname, '/');
    pid_t pid = atoi(fname, *end-fname);

    if (pid < 0) {
        klogf(Log_error, "procfs", "invalid directory name %s", fname);
        return -1;
    }

    proc_t *p = state.st_proc + pid;
    if (p->p_stat == FREE) {
        klogf(Log_error, "procfs", "unallocated pid %d", pid);
        return -1;
    }

    if (**end == '/') (*end)++;
    if (!**end) {
        // open folder /proc/pid
        kprintf("open folder\n");
        st->st_ino = pid * NB_PROC_FILE;
        goto success;
    }

    // get filename
    fname = *end;
    if (!ustrncmp(fname, "stat", 5)) {
        st->st_ino = pid * NB_PROC_FILE + STAT_PROC_FILE;
        goto success;
    } else if (!ustrncmp(fname, "cwd", 4)) {
        st->st_ino = pid * NB_PROC_FILE + CWD_PROC_FILE;
        goto success;
    }

    klogf(Log_error, "procfs", "unknown file %s", fname);
    return -1;
success:
    return st->st_ino;
}

// can't create file in this filesystem
int proc_create(void* super, ino_t ino, char *fname) { return -1; }

// TODO
int proc_seek(void* super, ino_t ino, off_t pos) { return 0; }

#define min(a, b) (a < b ? a : b)

int proc_read(void* super, ino_t ino, void *buf, size_t len) {
    if (ino == 0) return -1;

    pid_t pid = ino / 3;
    proc_t *p = state.st_proc + pid;

    int rc;
    switch (pid % NB_PROC_FILE) {
    case STAT_PROC_FILE:
        rc = min(len, sizeof(p->p_stat));
        memcpy(buf, &p->p_stat, rc);
        break;

    default:
        rc = -1;
        break;
    }

    return rc;
}

// not allowed to write
int proc_write(void* super, ino_t ino, void *buf, size_t len) { return -1; }

struct dirent *proc_readdir(struct dirent* dir) { return 0; }
