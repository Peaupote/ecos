#ifndef _U_MISC_H
#define _U_MISC_H


/*
 * Le premier argument étant découpé en 2^nstep parties de sz bits
 * (éventuellement complétés par des zéros)
 * Renvoie l'index (commence 0) de maximal d'une partie non nulle
 * Renvoie 0 si toutes les parties sont nulles
 */
#define GEN_BIT(N, T) \
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
}\
static inline void set_bit_##N(T* p, uint8_t i) {\
	*p |= ((T)1) << i;\
}\
static inline void clear_bit_##N(T* p, uint8_t i) {\
	*p &= ~(((T)1) << i);\
}\
static inline void clear_flag_##N(T* p, T f) {\
	*p &= ~f;\
}

GEN_BIT(64, uint64_t)
GEN_BIT(32, uint32_t)
GEN_BIT(8,   uint8_t)
#undef GEN_BIT

#include "paging.h"

#define GEN_COMP(T) \
	static inline T max_##T(T a, T b) {return b > a ? b : a;}\
	static inline void maxa_##T(T* a, T b) {if(b > *a) *a = b;}\
	static inline T min_##T(T a, T b) {return b < a ? b : a;}\
	static inline void mina_##T(T* a, T b) {if(b < *a) *a = b;}
GEN_COMP(int)
GEN_COMP(unsigned)
GEN_COMP(uint64_t)
GEN_COMP(uint32_t)
GEN_COMP(uint16_t)
GEN_COMP(size_t)
GEN_COMP(uint_ptr)
GEN_COMP(phy_addr)
#undef GEN_COMP

#define GEN_ABS(T) \
	static inline T abs_##T(T a) {return a >= 0 ? a : -a;}
GEN_ABS(int)
#undef GEN_ABS

#define GEN_ALIGN(N,T,U) \
	static inline T N(T addr, U align) {\
		return (addr % align)\
			? addr + align - (addr % align)\
			: addr;\
	}
GEN_ALIGN(align_to, uint_ptr, uint64_t)
GEN_ALIGN(align_to_size, size_t, size_t)
#undef GEN_ALIGN

// hash_str(B, hash_str(A, h)) = hash_str(concat(A,B), h)
#define GEN_HASH_STR(T, S, K) \
static inline T hash_str##S(const char* s, T h) {\
	while (*s) {\
		h  = (h << (K)) | (h >> (S-K));/*rol*/\
		h ^= *s;\
		++s;\
	}\
	return h;\
}\
static inline T hash_strn##S(const char* s, size_t n, T h) {\
	for (size_t i = 0; i < n; ++i) {\
		h  = (h << (K)) | (h >> (S-K));\
		h ^= s[i];\
		++s;\
	}\
	return h;\
}
#define EMPTY 
GEN_HASH_STR(unsigned char,  8,  3)
GEN_HASH_STR(uint64_t,      64, 23)
#undef EMPTY
#undef GEN_HASH_STR

#define GEN_SWAP(T) \
static inline void swap_##T(T* a, T* b) {\
	T tmp = *a; \
	*a = *b; \
	*b = tmp;\
}
GEN_SWAP(unsigned)
GEN_SWAP(int)
GEN_SWAP(uint32_t)
#undef GEN_SWAP

#endif
