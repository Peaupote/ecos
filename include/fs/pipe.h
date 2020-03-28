#ifndef _H_PIPE
#define _H_PIPE

#include <stddef.h>
#include <stdint.h>
#include <kernel/param.h>

struct pipe_inode {
    uint32_t pp_head;
    uint32_t pp_tail;

    uint16_t pp_uid;
    uint16_t pp_guid;
    uint32_t pp_size;

    uint32_t pp_ctime;
    uint32_t pp_mtime;
    char     pp_content[PIPE_SZ];
} pipes[NPIPE];

struct pipe_inode *pipe_alloc();
int pipe_read(struct pipe_inode *, void *dst, size_t len);
int pipe_write(struct pipe_inode *, void *src, size_t len);

#endif
