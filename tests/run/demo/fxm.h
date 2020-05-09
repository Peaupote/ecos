#ifndef _FXM_H
#define _FXM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define FXM_DIGITS      16
#define FXM_DIGITS_MASK 0xffff

typedef int32_t fxm_t;

static inline fxm_t fxm_of_int(int i) {
	return i << FXM_DIGITS;
}
static inline fxm_t fxm_one() {
	return 1 << FXM_DIGITS;
}
static inline int fxm_to_int(fxm_t f) {
	return f >> FXM_DIGITS;
}

void fxm_print(FILE* f, fxm_t  p, int d_lim);
bool fxm_scanu(FILE* f, fxm_t* r);

static inline fxm_t fxm_mul(fxm_t a, fxm_t b) {
	return (((int64_t) a) * ((int64_t)b)) >> FXM_DIGITS;
}
static inline fxm_t fxm_div(fxm_t a, fxm_t b) {
	return (((int64_t) a) << FXM_DIGITS) / ((int64_t)b);
}
fxm_t fxm_pow(fxm_t x, unsigned n);
static inline fxm_t fxm_pows(fxm_t x, int n) {
	return n > 0 ? fxm_pow(x, (unsigned)n)
				 : fxm_div(fxm_one(), fxm_pow(x, (unsigned)(-n)));
}
fxm_t fxm_sqrt(fxm_t x);
fxm_t fxm_exp(fxm_t t);
fxm_t fxm_ln(fxm_t t);

#endif
