#ifndef _BOOT_DEF_H
#define _BOOT_DEF_H

#include "../def.h"

#define BITS(A,B,V) ((V>>A) & ((1<<(B-A))-1))
#define BITSO(A,B,V,O) (BITS(A,B,V)<<O)
#define GDT_ENTRY_0(B,L) \
    (BITS(0,16,L) | (BITSO(0,16,B,16)))
#define GDT_ENTRY_1(B,L,A,F) \
    (BITS(16,24,B) | (A<<8) | BITSO(16,20,L,16) \
	| (F<<20) | (BITSO(24,32,B,24)))

#endif
