#include "fxm.h"

#include <stdio.h>
#include <ctype.h>

void fxm_print(FILE* f, fxm_t p, int d_lim) {
	if (p & FXM_DIGITS_MASK) {
		int32_t ep = p >> FXM_DIGITS;
		if (ep == -1)
			fprintf(f, "-0.");
		else
			fprintf(f, "%d.", ep < 0 ? ep + 1 : ep);
		p &= FXM_DIGITS_MASK;
		if (ep < 0)
			p = (1 << FXM_DIGITS) - p;
		// termine avec d_lim = -1 car 2 | 10
		for (int i = 0; i != d_lim && (p &= FXM_DIGITS_MASK); ++i) {
			p *= 10;
			fputc((p >> FXM_DIGITS) + '0', f);
		}
	} else {
		fprintf(f, "%d", p >> FXM_DIGITS);
	}
}

void fxm_mprint(FILE* f, fxm_t*  p, unsigned n, unsigned m, int d_lim) {
	for (unsigned i = 0; i < n; ++i)
		for (unsigned j = 0; j < m; ++p) {
			fxm_print(f, *p, d_lim);
			fprintf(f, ++j == m ? "\n": " ");
		}
}

bool fxm_scanu(FILE* f, fxm_t* r) {
	int c = fgetc(f);
	bool hr = false;
	if (isdigit(c)) {
		ungetc(c, f);
		if (fscanf(f, "%u", r) != 1) return false;
		*r <<= FXM_DIGITS;
		c = fgetc(f);
		hr = true;
	}
	if (c != '.' || (c = fgetc(f), !isdigit(c))) {
		ungetc(c, f);
		return hr;
	}
	uint64_t dgs = 0;
	uint64_t dgd = 1;
	do {
		dgs *= 10;
		dgs += c - '0';
		dgd *= 10;
	} while (isdigit(c = fgetc(f)));
	ungetc(c, f);
	while (dgs & (0xffffL << 0x30)) {
		dgs /= 10;
		dgd /= 10;
	}
	dgs <<= FXM_DIGITS;
	dgs /= dgd;
	*r  += dgs;
	return true;
}

fxm_t fxm_pow(fxm_t x, unsigned n) {
	fxm_t rt = fxm_one();
	while (n) {
		if (n & 1) rt = fxm_mul(rt, x);
		x = fxm_mul(x, x);
		n >>= 1;
	}
	return rt;
}

fxm_t fxm_exp(fxm_t t) {
	// exp(t) = exp(t / n)^n
	int n = ((t*2) >> FXM_DIGITS);
	if (!n) n = 1;
	t /= n;

	// exp(t) = sum (t^n / n!)
	fxm_t rt = 0;
	fxm_t z = fxm_one();
	long unsigned f = 1;
	for (unsigned i = 0; i < 10;) {
		rt += z / f;
		f  *= ++i;
		z   = fxm_mul(z, t);
	}

	return fxm_pows(rt, n);
}

static inline fxm_t fxm_ln_aux(fxm_t t) {
	// Méthode des tangentes de Newton
	fxm_t u = t - fxm_one();
	for (size_t i = 0; i < 10; ++i) {
		fxm_t v = fxm_exp(u);
		u += fxm_div(t - v, v);
	}
	return u;
}

fxm_t fxm_ln(fxm_t t) {
	bool inv = false;
	// ln(t) = -ln(1/t)
	if (t < 1) {
		inv = true;
		t = fxm_div(fxm_one(), t);
	}
	// ln(t) = ln (t / 2^n) + n * ln(2)
	unsigned n = 0;
	while (t >= fxm_of_int(2)) {
		++n;
		t >>= 1;
	}

	fxm_t  l = fxm_ln_aux(t);
	l += n * (fxm_t)0xb172;//ln(2)
	return inv ? -l : l;
}

fxm_t fxm_sqrt(fxm_t x) {
	fxm_t rt = 0;
	for (uint32_t ux = x < fxm_one() ? fxm_one() : x; ux; ux >>= 1) {
		fxm_t rt1 = rt + (fxm_t)ux;
		if (fxm_mul(rt1, rt1) <= x)
			rt = rt1;
	}
	return rt;
}

static const fxm_t cos_div[9] = {
	0xb504, 0xec83, 0xfb14,
	0xfec4, 0xffb1, 0xffec,
	0xfffb, 0xfffe, 0xffff
}; // cos (PI * 2^-n), 2 <= n <= 10

fxm_t fxm_cos(fxm_t t) {
	// 2 pi periodicite
	t -= fxm_to_int(fxm_div(t, 2 * FXM_PI)) * 2 * FXM_PI;
	// parité
	if (t < 0) t = -t;
	// on se ramène dans [0, pi/2]
	if (t > FXM_PI)
		t = 2 * FXM_PI - t;
	bool ng = false;
	if (t > FXM_PI / 2) {
		ng = true;
		t = FXM_PI - t;
	}
	if (t == FXM_PI / 2) return 0;

	fxm_t a  = 0,   b = FXM_PI / 2;
	fxm_t ca = fxm_one(), cb = 0;
	for (unsigned i = 0; i < 9; ++i) {
		fxm_t c  = (a + b) / 2;
		// cos ((a+b)/2) = (cos(a) + cos(b)) / (2 cos((a-b) / 2))
		fxm_t cc = fxm_div(ca + cb, 2 * cos_div[i]);
		if (t == c) return ng ? -cc : cc;
		if (t  > c) {
			a  = c;
			ca = cc;
		} else {
			b  = c;
			cb = cc;
		}
	}

	ca = (ca + cb) / 2;
	return ng ? -ca : ca;
}

fxm_t fxm_sin(fxm_t t) {
	return fxm_cos(FXM_PI / 2 - t);
}
