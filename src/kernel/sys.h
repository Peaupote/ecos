#ifndef _H_SYS
#define _H_SYS

#define SLEEP 0

#ifndef ASSEMBLY

#include "syscalls/sleep.h"

// TODO : convert cycle count into time
uint32_t sleep(uint32_t time);

void syscall_hdl(void);

#endif

#endif
