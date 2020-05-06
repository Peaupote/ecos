#include "sh.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <util/misc.h>

struct kwent {
	struct kwent* next;
	enum keywords kw;
	char name[];
};
#define NB_KEYWORDS     3
#define KEYWORD_MAX_LEN 5
#define KEYWORD_HT_MOD  32
#define G(K) static struct kwent kw_##K ={.next=0,.kw=KW_##K,.name=#K};
G(while)G(do)G(done)G(if)G(then)G(elif)G(else)G(fi)G(for)G(in)
#undef G
static struct kwent* kw_ht[KEYWORD_HT_MOD] = {0};
static void add_kw(struct kwent* e) {
	unsigned char h = hash_str8(e->name, 0) % KEYWORD_HT_MOD;
	e->next  = kw_ht[h];
	kw_ht[h] = e;
}
void init_parse() {
	add_kw(&kw_while);
	add_kw(&kw_do);
	add_kw(&kw_done);
	add_kw(&kw_if);
	add_kw(&kw_then);
	add_kw(&kw_elif);
	add_kw(&kw_else);
	add_kw(&kw_fi);
	add_kw(&kw_for);
	add_kw(&kw_in);
}
static int find_kw(const char* k) {
	unsigned char h = hash_str8(k, 0) % KEYWORD_HT_MOD;
	for (struct kwent* e = kw_ht[h]; e; e = e->next)
		if (!strcmp(e->name, k))
			return (int)e->kw;
	return -1;
}

enum pctype getctype(char c) {
	switch (c) {
		case '#':
			return CT_COM;

		case  ' ': case '\f': case '\t': case '\v':
			return CT_SPA;

		case '>': case '<':
			return CT_RED;

		case '\n': case '\r': case '\0':
		case '(': case ')':
		case ';': case '&': case '|':
			return CT_CNJ;

		case '\\': case '\'': case '"':
			return CT_ESC;

		case '$': case '~':
			return CT_VAR;

		default:
			return CT_NRM;
	}
}

void mk_ssrc(src_t* rt, const char* src) {
	rt->is_str = true;
	rt->ssrc   = src;
	rt->err    = 0;
	rt->pos_l  = 1;
	rt->pos_c  = 1;
	rt->pk2    = 0;
	rt->pkty   = PEEK_W0;
	rt->peekw  = NULL;
}

void mk_fsrc(src_t* rt, FILE* src) {
	rt->is_str = false;
	rt->fsrc   = src;
	rt->err    = 0;
	rt->pos_l  = 1;
	rt->pos_c  = 1;
	rt->pk2    = 0;
	rt->pkty   = PEEK_W0;
	rt->peekw  = NULL;
}

void destr_src(src_t* s) {
	if (s->pkty) {
		free(s->peekw);
		s->peekw = NULL;
		s->pkty  = PEEK_W0;
	}
}

static inline int src_err(src_t* s, int errline) {
	if (!s->err)
		s->err = errline;
	return 2;
}
#define ERR(S) src_err(S, __LINE__)

static inline char peek_1(src_t* s) {
	if (s->is_str) return *s->ssrc;
	if (s->pk2 == 0) {
		int c       = fgetc(s->fsrc);
		s->pk2      = 1;
		s->peek2[0] = c == EOF ? '\0' : (unsigned char) c;
	}
	return s->peek2[0];
}
static inline char peek_2(src_t* s) {
	if (s->is_str) return s->ssrc[1];
	while (s->pk2 < 2) {
		int c = fgetc(s->fsrc);
		s->peek2[s->pk2++] = c == EOF ? '\0' : (unsigned char) c;
	}
	return s->peek2[1];
}
static inline void peek_vald(src_t* s, size_t len) {
	if (s->is_str) {
		for (size_t i = 0; i < len; ++i) {
			if (s->ssrc[i] == '\n') {
				++s->pos_l;
				s->pos_c = 0;
			} else ++s->pos_c;
		}
		s->ssrc += len;
	} else {
		if (len == 0) return;
		for (size_t i = 0; i < len; ++i) {
			if (s->peek2[i] == '\n') {
				++s->pos_l;
				s->pos_c = 0;
			} else ++s->pos_c;
		}
		if (len == 1 && s->pk2 == 2) {
			s->peek2[0] = s->peek2[1];
			s->pk2 = 1;
		} else
			s->pk2 = 0;
	}
}
// must be PEEK_W0
static inline void put_peek_wd(src_t* s, enum src_peek_ty ty,
									const char* c, size_t len) {
	s->pkty  = ty;
	s->peekw = malloc(len * sizeof(char));
	memcpy(s->peekw, c, len);
}

void wait_nspace(src_t* s) {
	if (s->pkty) return;
	while (true) {
		char c = peek_1(s);
		if (!c) return;
		if (getctype(c) == CT_SPA)
			peek_vald(s, 1);
		else if (c == '#') {
			peek_vald(s, 1);
			while (c = peek_1(s), (c && c != '\n'))
				peek_vald(s, 1);
		} else if (c == '\\') {
			char c2 = peek_2(s);
			if (c2 != '\n') return;
			peek_vald(s, 2);
		} else return;
	}
}
static inline bool wait_conj(src_t* s) {
	if (s->pkty) return false;
	wait_nspace(s);
	return true;
}

static inline bool is_varname_char(char c) {
	switch (c) {
		case '{': case '}': return false;
		default:
			return c > ' ' && c <= '~';
	}
}

static inline char* read_varname(src_t* s) {
	cbuf_t buf;
	cbuf_init(&buf, 32);
	switch(s->pkty) {
		case PEEK_WDR: return NULL;
		case PEEK_KW: {
			cbuf_init(&buf, 32);
			size_t len = strlen(s->peekw);
			cbuf_putn(&buf, s->peekw, len);
			free(s->peekw);
			s->peekw = NULL;
			s->pkty  = PEEK_W0;
		} break;
		case PEEK_W0:
			cbuf_init(&buf, 32);
			wait_nspace(s);
		break;
	}
	char c;
	while (is_varname_char(c = peek_1(s))) {
		peek_vald(s, 1);
		cbuf_put(&buf, c);
	}
	if (buf.sz) {
		cbuf_put(&buf, '\0');
		return cbuf_shrink(&buf);
	} else {
		cbuf_destr(&buf);
		return NULL;
	}
}

//cursor after '$'
int read_var(src_t* s, cbuf_t* b) {
	cbuf_putn(b, "${", 2);
	char c = peek_1(s);
	if (c == '{') {
		peek_vald(s, 1);
		while (c = peek_1(s), c != '}') {
			if (!is_varname_char(c))
				return ERR(s);
			peek_vald(s, 1);
			cbuf_put(b, c);
		}
		peek_vald(s, 1);
	} else {
		if (!is_varname_char(c))
			return ERR(s);
		peek_vald(s, 1);
		cbuf_put(b, c);
		while(c = peek_1(s), isalnum(c)) {
			peek_vald(s, 1);
			cbuf_put(b, c);
		}
	}
	cbuf_put(b, '}');
	return 0;
}

//cursor on '\\'
int read_escpd_char(src_t* s, cbuf_t* b) {
	char c = peek_2(s);
	switch (c) {
		case '\0':
			return ERR(s);
		case '\n':
			peek_vald(s, 2);
			return 1;
		default:
			peek_vald(s, 2);
			cbuf_put(b, '\\');
			cbuf_put(b, c);
			return 0;
	}
}

// cursor after '"' / '\''
int read_escpd_str(char ty, src_t* s, cbuf_t* b) {
	cbuf_put(b, ty);
	char c;
	while (c = peek_1(s), c != ty) {
		if (c == '\0') return ERR(s);
		if (c == '\\') {
			int rt1 = read_escpd_char(s, b);
			if (rt1) return ERR(s);
		} else {
			peek_vald(s, 1);
			if (c == '$' && ty == '"')
				read_var(s, b);
			else
				cbuf_put(b, c);
		}
	}
	peek_vald(s, 1);
	cbuf_put(b, ty);
	return 0;
}

int read_word1(src_t* s, cbuf_t* b, bool acc_red) {
	const size_t sz0 = b->sz;
	switch (s->pkty) {
		case PEEK_WDR:
			if (acc_red) {
				size_t len = strlen(s->peekw);
				cbuf_putn(b, s->peekw, len);
				free(s->peekw);
				s->peekw = NULL;
				s->pkty  = PEEK_W0;
				return 0;
			} else return 1;
		case PEEK_KW:
			if (s->peekw) {
				size_t len = strlen(s->peekw);
				cbuf_putn(b, s->peekw, len);
				free(s->peekw);
				s->peekw = NULL;
			}
			s->pkty = PEEK_W0;
			break;
		case PEEK_W0:
			wait_nspace(s);
			break;
	}
	int rt1;

	while (true) {
		char c = peek_1(s);
		switch(getctype(c)) {
			case CT_CNJ: case CT_SPA: case CT_COM:
				return b->sz == sz0 ? 1 : 0;
			case CT_RED:
				if (b->sz == sz0) return 1;
				else if (acc_red) return 0;
				else {
					put_peek_wd(s, PEEK_WDR, b->c + sz0, b->sz - sz0);
					b->sz = sz0;
					return 1;
				}
			case CT_VAR:
				if (c == '$') {
					peek_vald(s, 1);
					rt1 = read_var(s, b);
					if (rt1) return ERR(s);
					break;
				}
				// FALLTHRU
			case CT_NRM:
				peek_vald(s, 1);
				cbuf_put(b, c);
				break;
			case CT_ESC:
				switch(c) {
					case '\\':
						rt1 = read_escpd_char(s, b);
						if (rt1 == 1)     return 0;
						else if (rt1 > 1) return ERR(s);
						break;
					case '\'': case '"':
						peek_vald(s, 1);
						rt1 = read_escpd_str(c, s, b);
						if (rt1) return ERR(s);
						break;
				}
				break;
		}
	}
}

static int read_keyw(src_t* s) {
	if (s->pkty == PEEK_KW) return s->keyw;
	else if (s->pkty) return -1;
	wait_nspace(s);
	size_t len = 0;
	char   buf[KEYWORD_MAX_LEN + 1];
	char c = peek_1(s);
	while ('a' <= c && c <= 'z') {
		c = peek_1(s);
		if (c < 'a' || 'z' < c) break;
		peek_vald(s, 1);
		buf[len++] = c;
		c = peek_1(s);
		if (len >= KEYWORD_MAX_LEN) break;
	}
	if (len == 0) {
		switch (c) {
			case '{':
				s->pkty  = PEEK_KW;
				s->peekw = NULL;
				return s->keyw = (int) KW_group_bg;
			case '}':
				s->pkty  = PEEK_KW;
				s->peekw = NULL;
				return s->keyw = (int) KW_group_ed;
			default:
				return -1;
		}
	} else {
		buf[len] = '\0';
		s->pkty  = PEEK_KW;
		s->peekw = strdup(buf);
		s->keyw = (getctype(c) <= CT_CNJ ? find_kw(buf) : -1);
		return s->keyw;
	}
}
static void keyw_vald(src_t* s) {
	switch (s->keyw) {
		case KW_group_bg: case KW_group_ed:
			peek_vald(s, 1);
		break;
		default:
			free(s->peekw);
			s->peekw = NULL;
			s->pkty  = PEEK_W0;
		break;
	}
}

static int read_words(src_t* s, char** rt) {
	cbuf_t buf;
	cbuf_init(&buf, 256);
	int rts = read_word1(s, &buf, false);
	if (rts) {
		cbuf_destr(&buf);
		return rts;
	}

	do {
		cbuf_put(&buf, ' ');
	} while (!(rts = read_word1(s, &buf, false)));
	
	if (rts > 1) {
		cbuf_destr(&buf);
		return rts;
	}

	buf.c[buf.sz - 1] = '\0';
	*rt = cbuf_shrink(&buf);
	return 0;
}

int parse_cmd_0(src_t* s, cmd_0_t* rt) {
	wait_nspace(s);
	if (!s->pkty && peek_1(s) == '(') {
		rt->ty  = C_SUB;
		peek_vald(s, 1);

		int rts = parse_cmd_3c(s, &rt->sub);
		if (rts > 1) return rts;

		wait_nspace(s);
		if (s->pkty || peek_1(s) != ')') {
			if (!rts) {
				destr_cmd_3(rt->sub);
				free(rt->sub);
				rt->sub = NULL;
				return ERR(s);
			}
		}
		peek_vald(s, 1);
		if(!rts)
			rt->sub->parent = NULL;
		else
			rt->sub = NULL;
		return 0;
	}

	rt->ty = C_BAS;
	return read_words(s, &rt->bas);
}

bool parse_fd(const char* wd, int* rt) {
	if (!isdigit(*wd)) return false;
	*rt = 0;
	do {
		*rt = 10 * (*rt) + *wd - '0';
	} while(isdigit(*++wd));
	return *wd == '\0';
}

int parse_redt(src_t* s) {
	char c = peek_1(s);
	int rt;
	if (c == '<')
		rt = IN_FILE;
	else if (c == '>' && peek_2(s) == '>') {
		peek_vald(s, 2);
		return OUT_APPEND;
	} else if (c == '>')
		rt = OUT_TRUNC;
	else return -1;

	peek_vald(s, 1);
	wait_nspace(s);
	c = peek_1(s);
	if (c == '&') {
		peek_vald(s, 1);
		return rt - 1;//dup
	} else return rt;
}

int parse_redir(src_t* s, redir_t* rt) {
	cbuf_t buf;
	cbuf_init(&buf, 64);
	int rts = read_word1(s, &buf, true);
	bool pfd = false;
	if (!rts) {
		cbuf_put(&buf, '\0');
		if (!parse_fd(buf.c, &rt->fd)) {
			cbuf_destr(&buf);
			return ERR(s);
		}
		pfd = true;
	} else if (rts > 1) {
		cbuf_destr(&buf);
		return ERR(s);
	}
	cbuf_destr(&buf);

	int ty = parse_redt(s);
	if (!~ty) return pfd ? ERR(s) : 1;
	rt->type = (enum redir_ty) ty;
	if (!pfd) rt->fd = ty <= IN_FILE ? 0 : 1;

	cbuf_init(&buf, 64);
	rts = read_word1(s, &buf, false);
	if (rts) {
		cbuf_destr(&buf);
		return ERR(s);
	}
	cbuf_put(&buf, '\0');
	switch (rt->type) {
		case IN_DUP: case OUT_DUP: {
			bool rfd = parse_fd(buf.c, &rt->src_fd);
			cbuf_destr(&buf);
			return rfd ? 0 : ERR(s);
		}
		case IN_FILE: case OUT_TRUNC: case OUT_APPEND: {
			rt->filewd = cbuf_shrink(&buf);
			return 0;
		}
	}
	return ERR(s);//never reached
}

// en sortie 0 src->pkty = PEEK_W0
int parse_cmd_1(src_t* s, cmd_1_t* rt) {
	int rts = parse_cmd_0(s, &rt->c);
	if (rts) return rts;

	redbuf_t rbuf;
	redbuf_init(&rbuf, 1);
	while (true) {
		redir_t* r = redbuf_mk(&rbuf);
		rts = parse_redir(s, r);
		if (rts == 1) {
			rt->r.redc = --rbuf.sz;
			rt->r.reds = redbuf_shrink(&rbuf);
			return 0;
		} else if (rts > 1){
			rt->r.redc = --rbuf.sz;
			rt->r.reds = redbuf_shrink(&rbuf);
			destr_cmd_1(rt);
			return rts;
		}
	}
}

// en sortie <= 1 src->pkty = PEEK_W0
int parse_cmd_2(src_t* s, cmd_2_t* rt) {
	size_t c_sp = 1;
	rt->cmds    = malloc(c_sp * sizeof(cmd_1_t));
	rt->cmdc    = 0;
	while (true) {
		if (rt->cmdc == c_sp) {
			c_sp    *= 2;
			rt->cmds = realloc(rt->cmds, c_sp * sizeof(cmd_1_t));
		}
		int rts = parse_cmd_1(s, rt->cmds + rt->cmdc);
		if (rts) {
			destr_cmd_2(rt);
			return (rt->cmdc && rts == 1) ? ERR(s) : rts;
		}
		++rt->cmdc;
		assert(s->pkty == PEEK_W0);
		wait_nspace(s);
		if (peek_1(s) != '|' || peek_2(s) == '|') {
			rt->cmds = realloc(rt->cmds, rt->cmdc * sizeof(cmd_1_t));
			return 0;
		}
		peek_vald(s, 1);
	}
}

static int parse_reds3(src_t* s, red_list_t* rt) {
	redbuf_t rbuf;
	redbuf_init(&rbuf, 2);

	while (true) {
		redir_t* r = redbuf_mk(&rbuf);
		int rts = parse_redir(s, r);
		if (rts > 1) {
			--rbuf.sz;
			for (size_t i = 0; i < rbuf.sz; ++i)
				destr_redir(rbuf.c + i);
			redbuf_destr(&rbuf);
			return rts;
		} else if (rts) {
			--rbuf.sz;
			break;
		}
	}

	if (rbuf.sz == 0) {
		redbuf_destr(&rbuf);
		return 1;
	}

	rt->redc = rbuf.sz;
	rt->reds = redbuf_shrink(&rbuf);
	return 0;
}

static int ev_reds3(src_t* s, cmd_3_t** rt) {
	red_list_t rl;
	int rts = parse_reds3(s, &rl);
	if (!rts) {
		cmd_3_t* rd = malloc(sizeof(cmd_3_t));
		rd->ty      = C_RED;
		rd->red.r   = rl;
		rd->red.c   = *rt;
		if (*rt) {
			(*rt)->sibnum = 0;
			(*rt)->parent = rd;
		}
		*rt = rd;
		return 0;
	} else if (rts == 1) {
		return *rt ? 0 : 1;
	} else {
		destr_cmd_3nl(rt);
		return ERR(s);
	}
}

static inline cmd_3_t* set_child_i(cmd_3_t* p, cmd_3_t* c, uint8_t sibnum) {
	if (c) {
		c->parent = p;
		c->sibnum = sibnum;
	}
	return c;
}

int parse_cmd_3a(src_t* s, cmd_3_t** rt) {
	int rts;
	int kw = read_keyw(s);
	switch (kw) {
		case KW_group_bg:
			keyw_vald(s);
			rts = parse_cmd_3c(s, rt);
			if (rts > 1) return rts;
			if (read_keyw(s) != KW_group_ed) {
				destr_cmd_3nl(rt);
				return ERR(s);
			}
			keyw_vald(s);
			return ev_reds3(s, rt);

		case KW_while: {
			keyw_vald(s);
			cmd_3_t *cnd, *bdy;
			if (parse_cmd_3c(s, &cnd) > 1
			||  read_keyw(s) != KW_do
			||  (keyw_vald(s), parse_cmd_3c(s, &bdy) > 1)
			||  read_keyw(s) != KW_done) {
				destr_cmd_3nl(&cnd);
				destr_cmd_3nl(&bdy);
				*rt = NULL;
				return ERR(s);
			}
			keyw_vald(s);

			cmd_3_t* r = *rt = malloc(sizeof(cmd_3_t));
			r->ty      = C_WHL;
			r->whl.cnd = set_child_i(r, cnd, 0);
			r->whl.bdy = set_child_i(r, bdy, 1);

			return ev_reds3(s, rt);
		}
		case KW_if: {
			keyw_vald(s);
			cmd_3_t* r = *rt = malloc(sizeof(cmd_3_t));
			
			while (true) {
				r->ty = C_IF;
				r->cif.brs[0] = r->cif.brs[1] = NULL;
				if (parse_cmd_3c(s, &r->cif.cnd) > 1
				||  read_keyw(s) != KW_then
				||  (keyw_vald(s), parse_cmd_3c(s, r->cif.brs) > 1)) {
					destr_cmd_3nl(rt);
					return ERR(s);
				}

				set_child_i(r, r->cif.cnd,    0);
				set_child_i(r, r->cif.brs[0], 1);

				switch (read_keyw(s)) {
					case KW_fi:
						keyw_vald(s);
						return ev_reds3(s, rt);
					case KW_elif:
						keyw_vald(s);
						r->cif.brs[1] = malloc(sizeof(cmd_3_t));
						r = set_child_i(r, r->cif.brs[1], 2);
						break;
					case KW_else:
						keyw_vald(s);
						if (parse_cmd_3c(s, r->cif.brs+1) > 1
						||  read_keyw(s) != KW_fi) {
							destr_cmd_3nl(rt);
							return ERR(s);
						} else {
							keyw_vald(s);
							set_child_i(r, r->cif.brs[1], 2);
							return ev_reds3(s, rt);
						}
					default:
						destr_cmd_3nl(&r->cif.cnd);
						destr_cmd_3nl(r->cif.brs);
						destr_cmd_3nl(rt);
						return ERR(s);
				}
			}
		}
		case KW_for: {
			keyw_vald(s);
			cmd_3_t *bdy;
			char *var = NULL, *wds = NULL;
			char c;
			if (!(var = read_varname(s))
			||  read_keyw(s) != KW_in
			||  (keyw_vald(s), read_words(s, &wds) > 1)
			||  !wait_conj(s) 
			||  (c = peek_1(s), c != '\n' && c != ';')
			||  (peek_vald(s, 1), read_keyw(s) != KW_do)
			||  (keyw_vald(s), parse_cmd_3c(s, &bdy) > 1)
			||  read_keyw(s) != KW_done) {
				if (var) free(var);
				if (wds) free(wds);
				destr_cmd_3nl(&bdy);
				return ERR(s);
			}
			keyw_vald(s);

			cmd_3_t* r  = *rt = malloc(sizeof(cmd_3_t));
			r->ty       = C_FOR;
			r->cfor.var = var;
			r->cfor.wds = wds;
			r->cfor.bdy = set_child_i(r, bdy, 0);

			return ev_reds3(s, rt);
		}
	}

	*rt = malloc(sizeof(cmd_3_t));
	(*rt)->ty = C_CM2;
	rts = parse_cmd_2(s, &(*rt)->cm2);
	if (rts) {
		free(*rt);
		*rt = NULL;
		return rts;
	}
	return 0;
}

static inline cmd_3_t* mk_bin_cmd_3(enum cmd_3_ty ty, cmd_3_t* l, cmd_3_t* r) {
	cmd_3_t* rt   = malloc(sizeof(cmd_3_t));
	rt->ty        = ty;
	rt->childs[0] = l;
	l->sibnum     = 0;
	l->parent     = rt;
	rt->childs[1] = r;
	r->sibnum     = 1;
	r->parent     = rt;
	return rt;
}

int parse_cmd_3b(src_t* s, cmd_3_t** rt) {
	cmd_3_t* cm = NULL;
	int rts = parse_cmd_3a(s, &cm);

	if (rts) return rts;

	while (true) {
		if (!wait_conj(s)) {
			*rt = cm;
			return 0;
		}
		const char c[2] = {peek_1(s), peek_2(s)};
		enum cmd_3_ty ty;
		if (*c == '&' && c[1] == '&')
			ty = C_AND;
		else if (*c == '|' && c[1] == '|')
			ty = C_OR;
		else {
			*rt = cm;
			return 0;
		}
		peek_vald(s, 2);

		cmd_3_t* c1;
		rts = parse_cmd_3a(s, &c1);
		if (rts) {
			destr_cmd_3(cm);
			free(cm);
			*rt = NULL;
			return rts == 1 ? ERR(s) : rts;
		}
		cm = mk_bin_cmd_3(ty, cm, c1);
	}
}

int parse_cmd_3c(src_t* s, cmd_3_t** rt) {
	cmd_3_t* cm = NULL;
	int rts;
	char c;

	goto loop_begin;	
	do {
		peek_vald(s, 1);
	loop_begin:
		if (read_keyw(s) >= KW_group_ed) break;

		cmd_3_t* c1;
		rts = parse_cmd_3b(s, &c1);
		if (rts > 1) {
			destr_cmd_3nl(&cm);
			return rts;
		}
		c = wait_conj(s) ? peek_1(s) : '\0';
		if (!rts) {
			if (c == '&') {
				cmd_3_t* c2   = malloc(sizeof(cmd_3_t));
				c2->ty        = C_BG;
				c2->childs[0] = c1;
				c1->sibnum    = 0;
				c1->parent    = c2;
				c1 = c2;
			}
			cm = cm ? mk_bin_cmd_3(C_SEQ, cm, c1) : c1;
		}
	} while (c == ';' || c == '\n' || c == '&');

	*rt = cm;
	return cm ? 0 : 1;
}
int parse_sh(src_t* s, cmd_3_t** rt) {
	int rts = parse_cmd_3c(s, rt);
	if (rts) return rts;
	if (peek_1(s) != '\0') {
		destr_cmd_3nl(rt);
		return ERR(s);
	}
	return 0;
}
