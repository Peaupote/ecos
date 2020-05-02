#include <libc/stdio.h>
#include <libc/ctype.h>
#include <libc/unistd.h>

#include <stdint.h>
#include <stdbool.h>

const uint8_t hexa_digit_of_char_tb0[16] =
    {0x16,0x16,0x16,0x00, 0x0f,0x16,0x0f,0x16,
     0x16,0x16,0x16,0x16, 0x16,0x16,0x16,0x16};
const uint8_t hexa_digit_of_char_tb1[16 * 3 - 9 - 1] =
    {0,1,2,3, 4,5,6,7, 8,9,~0,~0,   ~0,~0,~0,
     ~0,10,11,12, 13,14,15,~0, ~0,~0,~0,~0, ~0,~0,~0,~0,
     ~0,~0,~0,~0, ~0,~0,~0};
static uint8_t hexa_digit_of_char(char c) {
    return hexa_digit_of_char_tb1[
            hexa_digit_of_char_tb0[(c>>4)&0xf] + (c&0xf)];
}
static uint8_t dec_digit_of_char(char c) {
	if (c > '9' || c < '0') return ~0;
	return c - '0';
}
static uint8_t oct_digit_of_char(char c) {
	if (c > '7' || c < '0') return ~0;
	return c - '0';
}
typedef uint8_t (*digit_parser)(char);

//TODO: overflow
static ssize_t parse_unsigned(string_reader r, void* ri, size_t slim,
		digit_parser f, uint8_t base, uint64_t* rt) {
	*rt = 0;
	bool  hrd = false;
	uint8_t d;
	while (slim--) {
		char    ic;
		ssize_t st = r.read(ri, &ic, 1);
		if (st < 0) return st;
		if ( ! st ) return hrd ? 0 : 1;
		d = f(ic);
		if (d != (uint8_t)~0)
			*rt = (*rt) * base + d;
		else {
			r.unget(ri);
			return hrd ? 0 : 1;
		}
		hrd = true;
	}
	return hrd ? 0 : 1;
}

static ssize_t parse_signed(string_reader r, void* ri, size_t slim,
		digit_parser f, uint8_t base, int64_t* rt) {
	if (!slim) return 1;
	uint64_t urt;
	char    ic;
	ssize_t st = r.read(ri, &ic, 1);
	if (st < 0) return st;
	if ( ! st ) return 1;
	if (ic == '-') {
		uint64_t urt;
		st = parse_unsigned(r, ri, slim - 1, f, base, &urt);
		*rt = -urt;
		return st;
	} if (ic == '+')
		--slim;
	else
		r.unget(ri);
	st = parse_unsigned(r, ri, slim, f, base, &urt);
	*rt = urt;
	return st;
}

static ssize_t parse_signedi(string_reader r, void* ri, size_t slim,
		int64_t* rt) {
	if (!slim--) return 1;
	bool    neg = false;
	digit_parser f = &dec_digit_of_char;
	uint8_t   base = 10;
	char    ic;

	ssize_t st = r.read(ri, &ic, 1);
	if (st < 0) return st;
	if ( ! st ) return 1;

	switch (ic) {
		case '-':
			neg = true;
			// FALLTHRU
		case '+':
			if (!slim--) return 1;
			ssize_t st = r.read(ri, &ic, 1);
			if (st < 0) return st;
			if ( ! st ) return 1;
			break;
	}

	switch (ic) {
		case '0':
			if (!slim--) return 0;
			ssize_t st = r.read(ri, &ic, 1);
			if (st < 0) return st;
			if ( ! st ) return 0;
			if(ic == 'x' || ic == 'X') {
				f = &hexa_digit_of_char;
				base = 16;
				break;
			}
			f = &oct_digit_of_char;
			base = 8;
			// FALLTHRU
		default:
			r.unget(ri);
			++slim;
	}
	
	uint64_t urt;
	st = parse_unsigned(r, ri, slim, f, base, &urt);
	*rt = neg ? -urt : urt;
	return st;
}

static inline ssize_t skip_space(string_reader r, void *ri) {
	char ic;
	do {
		ssize_t st= r.read(ri, &ic, 1);
		if (st <  0) return st;
		if (st == 0) return 1;
	} while(isspace(ic));
	r.unget(ri);
	return 0;
}


ssize_t fpscanf(string_reader r, void* ri, const char* fmt, va_list ps) {
	size_t matched = 0;
	while (*fmt) {
		if (*fmt == '%') {
			bool ignore = false;
			uint8_t mod = 0;
			size_t slim = ~0;
			if (* ++fmt == '*') {
				++fmt;
				ignore = true;
			}
			if (isdigit(*fmt)) {
				slim = *fmt - '0';
				while(isdigit(*++fmt))
					slim = slim * 10 + *fmt - '0';
			}
			switch(*fmt) {
				case 'l':
					switch(* ++fmt) {
						case 'l':
							mod = 2;
							++fmt;
							break;
						default:
							mod = 1;
					}
					break;
				default:
					break;
			}

			int64_t  rd_nb = 0;
			uint64_t rd_nb_u;
			ssize_t  rd_nb_st;
			switch(* fmt) {
				
				case '%': {
					char ic;
					ssize_t st = r.read(ri, &ic, 1);
					if (st <  0) return st;
					if (st == 0) return matched;
					if (ic != '%') {
						r.unget(ri);
						return matched;
					}
					}break;

				case 'c':
					if (!~slim) slim = 1;
					if (ignore) {
						while (slim--) {
							char ic;
							ssize_t st= r.read(ri, &ic, 1);
							if (st <  0) return st;
							if (st == 0) return matched;
						}
					} else {
						char* dst = va_arg(ps, char*);
						while (slim--) {
							ssize_t st= r.read(ri, dst, 1);
							if (st <  0) return st;
							if (st == 0) return matched + 1;
							++dst;
						}
						++matched;
					}
					break;

				case 's': {
					char ic;
					do {
						ssize_t st= r.read(ri, &ic, 1);
						if (st <  0) return st;
						if (st == 0) return matched;
					} while(isspace(ic));
					r.unget(ri);

					if (ignore) {
						while(slim--) {
							ssize_t st= r.read(ri, &ic, 1);
							if (st <  0) return st;
							if (st == 0) return matched;
							if (isspace(ic)) {
								r.unget(ri);
								break;
							}
						}
						break;
					}

					char* dst = va_arg(ps, char*);
					*dst = ic;
					while(slim--) {
						ssize_t st= r.read(ri, dst, 1);
						if (st <  0) return st;
						if (st == 0) {
							*dst = '\0';
							return matched + 1;
						}
						if (isspace(*dst)) {
							r.unget(ri);
							break;
						}
						++dst;
					}
					* dst = '\0';
					++matched;
				}break;

				case 'i':
					skip_space(r, ri);
					rd_nb_st = parse_signedi(r, ri, slim, &rd_nb);
					goto signed_store;
				case 'd':
					skip_space(r, ri);
					rd_nb_st = parse_signed(r, ri, slim,
								&dec_digit_of_char, 10, &rd_nb);
				signed_store:
					if (rd_nb_st < 0) return rd_nb_st;
					if (rd_nb_st)     return matched;
					if (ignore)       break;
					++matched;
					switch(mod) {
						case 0:
							*va_arg(ps, int*)           = rd_nb;
							break;
						case 1:
							*va_arg(ps, long int*)      = rd_nb;
							break;
						case 2:
							*va_arg(ps, long long int*) = rd_nb;
							break;
					}
					break;
				case 'p':
					mod = 3;
					// FALLTHRU
				case 'x':
					skip_space(r, ri);
					rd_nb_st = parse_unsigned(r, ri, slim,
								&hexa_digit_of_char, 0x10, &rd_nb_u);
					if (rd_nb_st < 0) return rd_nb_st;
					if (rd_nb_st)     return matched;
					if (ignore)       break;
					++matched;
					switch(mod) {
						case 0:
							*va_arg(ps, unsigned*)           = rd_nb_u;
							break;
						case 1:
							*va_arg(ps, long unsigned*)      = rd_nb_u;
							break;
						case 2:
							*va_arg(ps, long long unsigned*) = rd_nb_u;
							break;
						case 3:
							*va_arg(ps, uint64_t*)           = rd_nb_u;
							break;
					}
					break;
			}

		} else if (isspace(*fmt)) {
			char ic;
			do {
				ssize_t st = r.read(ri, &ic, 1);
				if (st <  0) return st;
				if (st == 0) return matched;
			} while (isspace(ic));
			r.unget(ri);
		} else {
			char ic;
			ssize_t st = r.read(ri, &ic, 1);
			if (st <  0) return st;
			if (st == 0) return matched;
			if (ic != *fmt) {
				r.unget(ri);
				return matched;
			}
		}
		++fmt;
	}
	return matched;
}

struct sstrd_data {
	const char* str;
	size_t      ofs;
};

ssize_t sstrd_read(void *v_ins, char* out, size_t count) {
	struct sstrd_data* ins = (struct sstrd_data*)v_ins;
	size_t rt_c = 0;
	while (rt_c < count && ins->str[ins->ofs]) {
		*(out++) = ins->str[ins->ofs++];
		++rt_c;
	}
	return rt_c;
}
void sstrd_unget(void *v_ins) {
	--((struct sstrd_data*)v_ins)->ofs;
}

int sscanf(const char *str, const char *fmt, ...) {
	string_reader rd = {
		.read = &sstrd_read, .unget = &sstrd_unget
	};
	struct sstrd_data ins = {
		.str = str, .ofs = 0
	};
    va_list params;
    va_start(params, fmt);
	int rt = fpscanf(rd, &ins, fmt, params);
	va_end(params);
	return rt;
}

#ifndef __is_kernel

struct file_rddt {
	unsigned char last_c;
	FILE* f;
};

ssize_t file_read(void *p_dt, char* out, size_t count) {
	struct file_rddt* d = p_dt;
	size_t rc = 0;
	int     c;
	while (rc < count && (c = fgetc(d->f)) != EOF) {
		*out++ = (unsigned char)c;
		++rc;
	}
	if (rc) d->last_c = *(out - 1);
	return rc;
}
void file_unget(void *p_dt) {
	struct file_rddt* d = p_dt;
	if (ungetc(d->last_c, d->f) == EOF)
		fprintf(stderr, "error unget %c\n", d->last_c);
}

int fscanf(FILE* f, const char *fmt, ...) {
	string_reader rd = {
		.read = &file_read, .unget = &file_unget
	};
	struct file_rddt d = {
		.last_c = '#', .f = f
	};
    va_list params;
    va_start(params, fmt);
	int rt = fpscanf(rd, &d, fmt, params);
	va_end(params);
	return rt;
}

int scanf(const char *fmt, ...) {
	string_reader rd = {
		.read = &file_read, .unget = &file_unget
	};
	struct file_rddt d = {
		.last_c = '#', .f = stdin
	};
    va_list params;
    va_start(params, fmt);
	int rt = fpscanf(rd, &d, fmt, params);
	va_end(params);
	return rt;
}
#endif
