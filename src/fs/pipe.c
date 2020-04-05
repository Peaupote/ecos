#include <fs/pipe.h>
#include <kernel/kutil.h>
#include <kernel/file.h>
#include <libc/string.h>

struct pipe_inode *pipe_alloc() {
    size_t i;
    for (i = 0; i < PIPE_SZ; i++) {
        if (!pipes[i].pp_hard) break;
    }

    if (i == PIPE_SZ) return 0;
    struct pipe_inode *pipe = pipes + i;

    pipe->pp_head  = 0;
    pipe->pp_tail  = 0;
    pipe->pp_uid   = 0; // TODO
    pipe->pp_guid  = 0;
    pipe->pp_hard  = 1;
    pipe->pp_size  = 0;
    pipe->pp_ctime = 0; // TODO now
    pipe->pp_mtime = 0;

    return pipe;
}

int pipe_stat(struct pipe_inode *p, struct stat *st) {
    st->st_mode    = TYPE_FIFO;
    st->st_size    = p->pp_size;
    st->st_blksize = PIPE_SZ;
    st->st_blocks  = st->st_size >> 9;
    return 0;
}

int pipe_read(struct pipe_inode *p, void *buf, size_t len) {
    if (!p || !buf) return 0;
    if (p->pp_size == 0) return 0;

    size_t i;
    char *dst = (char*)buf;
    for (i = 0; i < len && p->pp_head < p->pp_tail; i++) {
        *dst++ = p->pp_content[p->pp_head++];
    }

    p->pp_size -= i;
    p->pp_mtime = 0; // TODO now

    return i;
}

int pipe_write(struct pipe_inode *p, const void *buf, size_t len) {
    if (!p || !buf) return 0;

    if (PIPE_SZ < p->pp_size + len) {
        klogf(Log_info, "pipe", "no enough space");
        return 0;
    }

    if (p->pp_tail + len > PIPE_SZ) {
        memmove(p->pp_content, p->pp_content + p->pp_head, p->pp_size);
        p->pp_head = 0;
        p->pp_tail = p->pp_size;
    }

    size_t i;
    char *src = (char*)buf;
    for (i = 0; i < len; i++) {
        p->pp_content[p->pp_tail++] = *src++;
    }

    p->pp_size += i;
    p->pp_mtime = 0; // TODO now
    return i;
}

int pipe_free(struct pipe_inode *p) {
    p->pp_hard = 0;
    return 1;
}
