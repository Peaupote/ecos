#ifndef _HD_PROC_H
#define _HD_PROC_H

#define PID_IDLE      0
#define PID_INIT      1
#define PID_STOP      2

#include <stddef.h>
#include <stdint.h>

typedef int32_t  pid_t;

#define PID_NONE (~((pid_t)0))

typedef uint8_t  priority_t;
typedef int32_t  cid_t;

enum chann_mode {
    UNUSED = 0,
    READ   = 1,
    WRITE  = 2,
    RDWR   = 3
};

#define O_RDONLY READ
#define O_WRONLY WRITE
#define O_RDWR   RDWR

#define O_CREAT     4
#define O_APPEND    8
#define O_TRUNC    16
#define O_NOFOLLOW 32

#endif
