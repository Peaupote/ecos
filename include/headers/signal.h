#ifndef _HD_SIGNAL_H
#define _HD_SIGNAL_H

#include <stdint.h>

typedef void (*sighandler_t)(int);

// sigid -> 1 << sigid
typedef int32_t sigset_t;

// 1 <= signum <= SIG_COUNT
// 0 <= sigid = signum - 1 < SIG_COUNT
#define SIG_COUNT 31

// masques de sigid
#define SIG_NOTCTB 0x00010100 // SIGKILL + SIGSTOP
#define SIG_DFLKIL 0x00000102 // SIGKILL + SIGINT
#define SIG_DFLSTP 0x00030000 // SIGSTOP + SIGTSTP
#define SIG_DFLCNT 0x00040000 // SIGCONT

#define SIG_IGN ((sighandler_t)0)
#define SIG_DFL ((sighandler_t)1)
#define SIG_ERR ((sighandler_t)~(uint64_t)0)

// signums
#define SIGINT   2
#define SIGKILL  9
#define SIGSTOP 17
#define SIGTSTP 18
#define SIGCONT 19

#endif
