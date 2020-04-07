#ifndef _HD_SIGNAL_H
#define _HD_SIGNAL_H

#include <stdint.h>

typedef void (*sighandler_t)(int);

// sigid -> 1 << sigid
typedef int32_t sigset_t;

// 1 <= signum <= SIG_COUNT
// 0 <= sigid = signum - 1 < SIG_COUNT
#define SIG_COUNT 31

#define SIG_NOTCTB 0x10100 //SIGKILL + SIGSTOP
#define SIG_DFLKIL 0x00102 //SIGKILL + SIGINT

#define SIG_IGN ((sighandler_t)0)
#define SIG_DFL ((sighandler_t)1)
#define SIG_ERR ((sighandler_t)~(uint64_t)0)

// signums
#define SIGINT   2
#define SIGKILL  9
#define SIGSTOP 17

#endif
