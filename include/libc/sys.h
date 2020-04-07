#ifndef _LIBC_SYS_H
#define _LIBC_SYS_H

#include <headers/signal.h>

void     _libc_init();

int      _setpriority(int prio);
int      _getpriority();
int      setpriority(int prio);
int      getpriority();

int      debug_block(int v);


void     _sigsethnd(sighandler_t);
void     _sigreturn();
uint8_t  _signal(int sigid, uint8_t hnd);

// Appel avec privilège ring 1

void     r1_prires(uint16_t new_ring);

#endif
