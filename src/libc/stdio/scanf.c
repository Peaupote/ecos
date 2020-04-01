#include <libc/stdio.h>
#include <libc/ctype.h>
#include <libc/unistd.h>

#include <stdint.h>
#include <stdbool.h>

uint8_t hexa_digit_of_char_tb0[16] =
    {0x16,0x16,0x16,0x00, 0x0f,0x16,0x0f,0x16,
     0x16,0x16,0x16,0x16, 0x16,0x16,0x16,0x16};
uint8_t hexa_digit_of_char_tb1[16 * 3 - 9 - 1] =
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
typedef uint8_t (*digit_parser)(char);

//TODO: overflow
static ssize_t parse_unsigned(string_reader r, void* ri,
		digit_parser f, uint8_t base, uint64_t* rt) {
	*rt = 0;
	bool  hrd = false;
	uint8_t d;
	do {
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
	} while(true);
}

static ssize_t parse_signed(string_reader r, void* ri,
		digit_parser f, uint8_t base, int64_t* rt) {
	uint64_t urt;
	char    ic;
	ssize_t st = r.read(ri, &ic, 1);
	if (st < 0) return st;
	if ( ! st ) return 1;
	if (ic == '-') {
		uint64_t urt;
		st = parse_unsigned(r, ri, f, base, &urt);
		*rt = -urt;
		return st;
	} else if (ic != '+')
		r.unget(ri);
	st = parse_unsigned(r, ri, f, base, &urt);
	*rt = urt;
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
		
			uint8_t mod = 0;
			switch(* ++fmt) {
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

			int64_t  rd_nb;
			uint64_t rd_nb_u;
			ssize_t  rd_nb_st;
			switch(* fmt) {

				case 's': {
					char ic;
					do {
						ssize_t st= r.read(ri, &ic, 1);
						if (st <  0) return st;
						if (st == 0) return matched;
					} while(isspace(ic));
					char* dst = va_arg(ps, char*);
					*dst = ic;
					do {
						++dst;
						ssize_t st= r.read(ri, dst, 1);
						if (st <  0) return st;
						if (st == 0) {
							*dst = '\0';
							return matched + 1;
						}
						if (isspace(*dst)) {
							*dst = '\0';
							r.unget(ri);
							++matched;
							break;
						}
					} while(true);
				}break;

				case 'd':
					skip_space(r, ri);
					rd_nb_st = parse_signed(r, ri, 
								&dec_digit_of_char, 10, &rd_nb);
					if (rd_nb_st < 0) return rd_nb_st;
					if (rd_nb_st) return matched;
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
				case 'x':
					skip_space(r, ri);
					rd_nb_st = parse_unsigned(r, ri, 
								&hexa_digit_of_char, 0x10, &rd_nb_u);
					if (rd_nb_st < 0) return rd_nb_st;
					if (rd_nb_st) return matched;
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
bool stdin_hunget = false;
char stdin_last;

ssize_t stdin_read(void *none __attribute__((unused)), char* out,
					size_t count) {
	if (stdin_hunget) {
		stdin_hunget = false;
		return stdin_last;
	}
	ssize_t rt = read(0, out, count);
	if (rt > 0)
		stdin_last = out[rt - 1];
	return rt;
}
void stdin_unget(void *none __attribute__((unused))) {
	stdin_hunget = true;
}

int scanf(const char *fmt, ...) {
	string_reader rd = {
		.read = &stdin_read, .unget = &stdin_unget
	};
    va_list params;
    va_start(params, fmt);
	int rt = fpscanf(rd, NULL, fmt, params);
	va_end(params);
	return rt;
}
#endif
