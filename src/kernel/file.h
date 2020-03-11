#ifndef _H_FILE
#define _H_FILE

#include <stddef.h>
#include <stdint.h>
#include "param.h"

typedef uint64_t bid_t;
typedef uint64_t ino_t;
typedef uint64_t pos_t;

struct file {
    ino_t    f_inode; // inode number
    uint64_t f_count; // reference count
    uint16_t f_mode;  // access mode of file
};

typedef struct buffer {
    ino_t    buf_inode;

    // remember global position in the file
    // and size of the loaded segment
    // size 0 mean unused
    pos_t    buf_offset;
    size_t   buf_size;

    // content of the file at offset...offset+BUFSIZE
    uint8_t  buf_content[BUFSIZE];

    // pointers to somewhere else in the file
    // should have some binary tree structure
    struct buffer *buf_pv, *buf_nx;
} buf_t;

buf_t *load_buffer(ino_t file, pos_t offset);

#endif
