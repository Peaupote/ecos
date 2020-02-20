#ifndef _GDT_H
#define _GDT_H

#define BITSUB(A,B,V) ((V>>A) & ((1<<(B-A))-1))
#define BITSUBO(A,B,V,O) (BITSUB(A,B,V)<<O)
#define GDT_ENTRY_0(B,L) \
    (BITSUB(0,16,L) | (BITSUBO(0,16,B,16)))
#define GDT_ENTRY_1(B,L,A,F) \
    (BITSUB(16,24,B) | (A<<8) | BITSUBO(16,20,L,16) \
	| (F<<20) | (BITSUBO(24,32,B,24)))

#endif
