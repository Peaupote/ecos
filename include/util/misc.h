#ifndef _U_MISC_H
#define _U_MISC_H

//reinterpret_cast
#define rei_cast(T, V) (*(T*)(&(V)))

/*
 * Le premier argument étant découpé en 2^nstep parties de sz bits
 * (éventuellement complétés par des zéros)
 * Renvoie l'index (commence 0) de maximal d'une partie non nulle
 * Renvoie 0 si toutes les parties sont nulles
 */
#define GEN_FBIT(N, T) \
static inline uint8_t find_bit_##N(T p, uint8_t sz, uint8_t nstep) {\
    T p2;\
    uint8_t i = 0;\
\
    for (uint8_t j = nstep; j--;) {\
        p2 = p >> (sz << j);\
        i |= p2 ? (1<<j) : 0;\
        p  = p2 ? p2     : p;\
    }\
\
    return i;\
}

GEN_FBIT(64, uint64_t)
GEN_FBIT(32, uint32_t)
GEN_FBIT(8,   uint8_t)
#undef GEN_FBIT

#include "paging.h"

#define GEN_COMP(T) \
	static inline T max_##T(T a, T b) {return b > a ? b : a;}\
	static inline void maxa_##T(T* a, T b) {if(b > *a) *a = b;}\
	static inline T min_##T(T a, T b) {return b < a ? b : a;}\
	static inline void mina_##T(T* a, T b) {if(b < *a) *a = b;}
GEN_COMP(int)
GEN_COMP(uint64_t)
GEN_COMP(uint32_t)
GEN_COMP(size_t)
GEN_COMP(uint_ptr)
GEN_COMP(phy_addr)
#undef GEN_COMP

#endif
