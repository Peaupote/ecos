#ifndef _U_GDT_H
#define _U_GDT_H

#define BITSUB(A,B,V) (((V)>>A) & ((1<<((B)-(A)))-1))
#define BITSUBO(A,B,V,O) (BITSUB(A,B,V)<<O)
#define GDT_ENTRY_0(B,L) \
    (BITSUB(0,16,L) | (BITSUBO(0,16,B,16)))
#define GDT_ENTRY_1(B,L,A,F) \
    (BITSUB(16,24,B) | ((A)<<8) | BITSUBO(16,20,L,16) \
	| ((F)<<20) | (BITSUBO(24,32,B,24)))
#define GDT_ENTRY_64(B,L,A,F) \
	GDT_ENTRY_0(B,L) | (((uint64_t)GDT_ENTRY_1(B,L,A,F))<<32)

//Access
#define GDT_ACC_A (1<<0)
//Read (for code)
#define GDT_ACC_R (1<<1)
//Write (for data)
#define GDT_ACC_W (1<<1)
//Direction (for data)
#define GDT_ACC_D (1<<2)
//Conforming (for code)
#define GDT_ACC_C (1<<2)
//Executable
#define GDT_ACC_E (1<<3)
//System
#define GDT_ACC_S (1<<4)
//Privilege - 2 bits
#define GDT_ACC_Pr(R) ((R)<<5)
//Present
#define GDT_ACC_P (1<<7)

//Granularity
#define GDT_FLAG_G (1<<3)
//Size
#define GDT_FLAG_S (1<<2)
//Long
#define GDT_FLAG_L (1<<1)

//Segment Selector (Index, Privilege)
#define SEG_SEL(I, P) ((I)|(P))

#endif
