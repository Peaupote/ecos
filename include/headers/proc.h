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
    UNUSED,
    READ,
    WRITE,
    RDWR,
    STREAM_IN,
    STREAM_OUT
};

#endif
