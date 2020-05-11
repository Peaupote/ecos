#ifndef _FXM_H
#define _FXM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define FXM_DIGITS      16
#define FXM_DIGITS_MASK 0xffff

typedef int32_t fxm_t;

#define FXM_PI          ((fxm_t)0x3243f)

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
void fxm_mprint(FILE* f, fxm_t*  p, unsigned n, unsigned m, int d_lim);

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

fxm_t fxm_cos(fxm_t t);
fxm_t fxm_sin(fxm_t t);

// (k, m) = (k, l) * (l, m)
static inline void fxm_mmul(const fxm_t* a, const fxm_t* b, fxm_t* rt,
		unsigned k, unsigned l, unsigned m) {
	for (unsigned i = 0; i < k; ++i)
		for (unsigned j = 0; j < m; ++j, ++rt) {
			*rt = 0;
			for (unsigned s = 0; s < l; ++s)
				*rt += fxm_mul(a[i * l + s], b[s * m + j]);
		}
}

static inline bool fxm_mslv2(const fxm_t a[4], fxm_t v[2], fxm_t rt[2]) {
	fxm_t t[4];
	if (a[3] && v[0]) {
		t[0] = fxm_div(a[0], v[0]) 
			 - fxm_mul(fxm_div(a[1], a[3]), fxm_div(a[2],v[0]));
		if (!t[0]) return false;
		t[0] = fxm_div(fxm_one(), t[0]);
	} else t[0] = 0;
	if (a[1] && v[1]) {
		t[1] = fxm_mul(fxm_div(a[0], a[1]), fxm_div(a[3],v[1]))
			 - fxm_div(a[2], v[1]);
		if (!t[1]) return false;
		t[1] = fxm_div(fxm_one(), t[1]);
	} else t[1] = 0;
	if (a[2] && v[0]) {
		t[2] = fxm_mul(fxm_div(a[0], a[2]), fxm_div(a[3],v[0]))
			 - fxm_div(a[1], v[0]);
		if (!t[2]) return false;
		t[2] = fxm_div(fxm_one(), t[2]);
	} else t[2] = 0;
	if (a[0] && v[1]) {
		t[3] = fxm_div(a[3], v[1]) 
			 - fxm_mul(fxm_div(a[1], a[0]), fxm_div(a[2],v[1]));
		if (!t[3]) return false;
		t[3] = fxm_div(fxm_one(), t[3]);
	} else t[3] = 0;

	rt[0] = + t[0] - t[1];
	rt[1] = - t[2] + t[3];
	return true;
}

static inline void fxm_mid(fxm_t* rt, unsigned d) {
	for (unsigned i = 0; i < d; ++i)
		for (unsigned j = 0; j < d; ++j, ++rt)
			*rt = i == j ? fxm_one() : 0;
}

static inline void fxm_mtrl(fxm_t* rt, const fxm_t* v) {
	fxm_mid(rt, 4);
	for (unsigned i = 0; i < 3; ++i)
		rt[i * 4 + 3] = v[i];
}

// ordre d'application de v: x puis y puis z
static inline void fxm_mrot(fxm_t* rt, const fxm_t* v) {
	fxm_t cx = fxm_cos(v[0]), sx = fxm_sin(v[0]),
	      cy = fxm_cos(v[1]), sy = fxm_sin(v[1]),
	      cz = fxm_cos(v[2]), sz = fxm_sin(v[2]);
	
	rt[ 0] =  cz;
	rt[ 1] = -fxm_mul(sz, cx) + fxm_mul(cz, fxm_mul(sy, sx));
	rt[ 2] =  fxm_mul(sz, sx) + fxm_mul(cz, fxm_mul(sy, cx));
	rt[ 3] =  0;

	rt[ 4] =  fxm_mul(sz, cy);
	rt[ 5] =  fxm_mul(cz, cx) + fxm_mul(sz, fxm_mul(sy, sx));
	rt[ 6] = -fxm_mul(cz, sx) + fxm_mul(sz, fxm_mul(sy, cx));
	rt[ 7] =  0;

	rt[ 8] = -sy;
	rt[ 9] = fxm_mul(cy, sx);
	rt[10] = fxm_mul(cy, cx);
	rt[11] = 0;

	rt[12] = rt[13] = rt[14] = 0;
	rt[15] = fxm_one();
}

static inline void fxm_mproj_h(fxm_t* rt,
		unsigned w, unsigned h, fxm_t zoom,
		fxm_t min_d, fxm_t max_d) {
	fxm_t w2 = fxm_of_int(w) / 2,
		  h2 = fxm_of_int(h) / 2;

	for (unsigned i = 0; i < 16; ++i) rt[i] = 0;

	// X left
	rt[0] =  fxm_mul(zoom, w2);
	rt[3] =  w2;

	// Z up
	rt[6] = -fxm_mul(zoom, h2);
	rt[7] =  h2;

	// Y depth
	rt[9 ] = 255 * fxm_div(fxm_one(), (max_d - min_d));
	rt[11] = - fxm_mul(rt[9], min_d);
}

static inline bool fxm_normalize(const fxm_t* a, fxm_t* rt, unsigned d) {
	fxm_t mterm = 0;
	for (unsigned i = 0; i < d; ++i) {
		if (a[i] > mterm) mterm = a[i];
		else if (-a[i] > mterm) mterm = -a[i];
	}
	if (!mterm) return false;
	fxm_t norm = 0;
	for (unsigned i = 0; i < d; ++i) {
		fxm_t b = fxm_div(a[i], mterm);
		norm += fxm_mul(b, b);
	}
	norm = fxm_mul(mterm, fxm_sqrt(norm));
	for (unsigned i = 0; i < d; ++i)
		rt[i] = fxm_div(a[i], norm);
	return true;
}

static inline void fxm_vprod(const fxm_t* a, const fxm_t* b, fxm_t* rt) {
	rt[0] = a[1] * b[2] - b[1] * a[2];
	rt[1] = a[2] * b[0] - b[2] * a[0];
	rt[2] = a[0] * b[1] - b[0] * a[1];
}

#endif
