#include "sh.h"

#include <ctype.h>
#include <stdlib.h>

enum pctype getctype(char c) {
	switch (c) {
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

void wait_nspace(const char** c) {
	while (**c && getctype(**c) == CT_SPA
			&& (**c != '\\' || (++*c, **c != '\n')))
		++*c;
}

static inline bool is_varname_char(char c) {
	switch (c) {
		case '{': case '}': return false;
		default:
			return c > ' ' && c <= '~';
	}
}

int read_var(const char** src, cbuf_t* b) {
	cbuf_putn(b, "${", 2);
	if (*++*src == '{') {
		while (*++*src != '}') {
			if (!is_varname_char(**src))
				return 2;
			cbuf_put(b, **src);
		}
	} else {
		if (!is_varname_char(**src))
			return 2;
		cbuf_put(b, **src);
		while(isalnum(*++*src))
			cbuf_put(b, **src);
		--*src;
	}
	cbuf_put(b, '}');
	return 0;
}

int read_escpd_char(const char** src, cbuf_t* b) {
	char c = *++*src;
	switch (c) {
		case '\0':
			return 2;
		case '\n':
			return 1;
		default:
			cbuf_put(b, '\\');
			cbuf_put(b, c);
			return 0;
	}
}

int read_escpd_str(const char** src, cbuf_t* b) {
	char ty = **src;
	cbuf_put(b, ty);
	char c;
	while ((c = *++*src) != ty) {
		if (c == '\0') return 2;
		if (c == '\\') {
			int rt1 = read_escpd_char(src, b);
			if (rt1) return 2;
		} else if (c == '$' && ty == '"')
			read_var(src, b);
		else
			cbuf_put(b, c);
	}
	cbuf_put(b, ty);
	return 0;
}

int read_word1(const char** src, cbuf_t* b) {
	wait_nspace(src);
	const char* src0 = *src;
	const size_t sz0 = b->sz;
	int rt1;

	while (true) {
		char c = **src;
		switch(getctype(c)) {
			case CT_CNJ: case CT_SPA:
				return *src == src0 ? 1 : 0;
			case CT_RED:
				*src  = src0;
				b->sz = sz0;
				return 1;
			case CT_VAR:
				if (c == '$') {
					rt1 = read_var(src, b);
					if (rt1) return 2;
					break;
				}
				// FALLTHRU
			case CT_NRM:
				cbuf_put(b, c);
				break;
			case CT_ESC:
				switch(c) {
					case '\\':
						rt1 = read_escpd_char(src, b);
						if (rt1 == 1) {
							++*src;
							return 0;
						} else if (rt1 > 1) return 2;
						break;
					case '\'': case '"':
						rt1 = read_escpd_str(src, b);
						if (rt1) return 2;
						break;
				}
				break;
		}
		++*src;
	}
}

int parse_cmd_0(const char** cr, cmd_0_t* rt) {
	//printf("read_cmd_0 @ line+%d\n", *cr - line);
	wait_nspace(cr);
	if (**cr == '(') {
		rt->ty  = C_SUB;
		++*cr;

		int rts = parse_cmd_3c(cr, &rt->sub);
		if (rts > 1) return rts;

		wait_nspace(cr);
		if (**cr != ')') {
			if (!rts) {
				destr_cmd_3(rt->sub);
				free(rt->sub);
				rt->sub = NULL;
				return 2;
			}
		}
		rt->sub->parent = NULL;
		++*cr;
		return rts;
	}

	rt->ty = C_BAS;
	cbuf_t buf;
	cbuf_init(&buf, 256);
	int rts = read_word1(cr, &buf);
	if (rts) {
		free(buf.c);
		return rts;
	}

	do {
		cbuf_put(&buf, ' ');
	} while (!(rts = read_word1(cr, &buf)));
	if (rts > 1) {
		free(buf.c);
		return rts;
	}
	buf.c[buf.sz - 1] = '\0';
	rt->bas = cbuf_shrink(&buf);
	return 0;
}

bool parse_fd(const char** cr, int* rt) {
	if (isdigit(**cr)) {
		*rt = 0;
		do {
			*rt = 10 * (*rt) + **cr - '0';
		} while(isdigit(*++*cr));
		return true;
	}
	return false;
}

int parse_redt(const char** cr) {
	const char* c = *cr;
	int rt;
	if (*c == '<')
		rt = IN_FILE;
	else if (*c == '>' && c[1] == '>') {
		*cr = c + 2;
		return OUT_APPEND;
	} else if (*c == '>')
		rt = OUT_TRUNC;
	else return -1;

	++c;
	wait_nspace(&c);
	if (*c == '&') {
		*cr = c + 1;
		return rt - 1;//dup
	} else {
		*cr = c;
		return rt;
	}
}

int parse_redir(const char** cr, redir_t* rt) {
	//printf("parse_redir @ line+%d\n", *cr - line);
	wait_nspace(cr);
	
	if (**cr != '<' && **cr != '>') {
		if (!parse_fd(cr, &rt->fd))
			return 1;
		int ty = parse_redt(cr);
		if (!~ty) return 2;
		rt->type = (enum redir_ty) ty;
	} else {
		int ty = parse_redt(cr);
		if (!~ty) return 1;
		rt->type = (enum redir_ty) ty;
		rt->fd = ty <= IN_FILE ? 0 : 1;
	}

	switch (rt->type) {
		case IN_DUP: case OUT_DUP:
			wait_nspace(cr);
			return parse_fd(cr, &rt->src_fd) ? 0 : 2;

		case IN_FILE: case OUT_TRUNC: case OUT_APPEND: {
			cbuf_t buf;
			cbuf_init(&buf, 64);
			int rts = read_word1(cr, &buf);
			if (rts) {
				free(buf.c);
				return 2;
			}
			cbuf_put(&buf, '\0');
			rt->filewd = cbuf_shrink(&buf);
			return 0;
		}
	}
	return 2;//never reached
}

int parse_cmd_1(const char** cr, cmd_1_t* rt) {
	//printf("parse_cmd_1 @ line+%d\n", *cr - line);
	int rts = parse_cmd_0(cr, &rt->cmd);
	if (rts) return rts;

	size_t rd_sp = 1;
	rt->reds     = malloc(rd_sp * sizeof(redir_t));
	rt->redc = 0; 
	while (true) {
		if (rt->redc == rd_sp) {
			rd_sp   *= 2;
			rt->reds = realloc(rt->reds, rd_sp * sizeof(redir_t));
		}
		rts = parse_redir(cr, rt->reds + rt->redc);
		if (!rts) ++rt->redc;
		else if (rts == 1) {
			rt->reds = realloc(rt->reds, rt->redc * sizeof(redir_t));
			return 0;
		} else {
			destr_cmd_1(rt);
			return rts;
		}
	}
}

int parse_cmd_2(const char** cr, cmd_2_t* rt) {
	//printf("parse_cmd_2 @ line+%d\n", *cr - line);
	size_t c_sp = 1;
	rt->cmds    = malloc(c_sp * sizeof(cmd_1_t));
	rt->cmdc    = 0;
	while (true) {
		if (rt->cmdc == c_sp) {
			c_sp    *= 2;
			rt->cmds = realloc(rt->cmds, c_sp * sizeof(cmd_1_t));
		}
		int rts = parse_cmd_1(cr, rt->cmds + rt->cmdc);
		if (rts) {
			destr_cmd_2(rt);
			return (rt->cmdc && rts == 1) ? 2 : rts;
		}
		++rt->cmdc;
		wait_nspace(cr);
		if (**cr != '|' || (*cr)[1] == '|') {
			rt->cmds = realloc(rt->cmds, rt->cmdc * sizeof(cmd_1_t));
			return 0;
		}
		++*cr;
	}
}

int parse_cmd_3a(const char** cr, cmd_3_t** rt) {
	//printf("parse_cmd_3a @ line+%d\n", *cr - line);
	wait_nspace(cr);
	if (**cr == '{') {
		++*cr;
		int rts = parse_cmd_3c(cr, rt);
		if (rts) return rts == 1 ? 2 : rts;
		wait_nspace(cr);
		if (**cr != '}') {
			destr_cmd_3(*rt);
			free(*rt);
			return 2;
		}
		++*cr;
		return 0;
	}

	*rt = malloc(sizeof(cmd_3_t));
	(*rt)->ty = C_CM2;
	int rts = parse_cmd_2(cr, &(*rt)->cm2);
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

int parse_cmd_3b(const char** cr, cmd_3_t** rt) {
	//printf("parse_cmd_3b @ line+%d\n", *cr - line);
	cmd_3_t* cm = NULL;
	int rts = parse_cmd_3a(cr, &cm);

	if (rts) return rts;

	while (true) {
		wait_nspace(cr);
		const char *c = *cr;
		enum cmd_3_ty ty;
		if (*c == '&' && c[1] == '&')
			ty = C_AND;
		else if (*c == '|' && c[1] == '|')
			ty = C_OR;
		else {
			*rt = cm;
			return 0;
		}
		*cr += 2;

		cmd_3_t* c1;
		rts = parse_cmd_3a(cr, &c1);
		if (rts) {
			destr_cmd_3(cm);
			free(cm);
			return rts == 1 ? 2 : rts;
		}
		cm = mk_bin_cmd_3(ty, cm, c1);
	}
}

int parse_cmd_3c(const char** cr, cmd_3_t** rt) {
	//printf("parse_cmd_3c @ line+%d\n", *cr - line);
	cmd_3_t* cm = NULL;
	int rts;

	goto loop_begin;	
	do {
		++*cr;
	loop_begin:
		wait_nspace(cr);
		if (**cr == '}')
			break;

		cmd_3_t* c1;
		rts = parse_cmd_3b(cr, &c1);
		if (rts > 1) {
			if (cm) {
				destr_cmd_3(cm);
				free(cm);
			}
			return rts;
		}
		wait_nspace(cr);
		if (!rts) {
			if (**cr == '&') {
				cmd_3_t* c2   = malloc(sizeof(cmd_3_t));
				c2->ty        = C_BG;
				c2->childs[0] = c1;
				c1->sibnum    = 0;
				c1->parent    = c2;
				c1 = c2;
			}
			cm = cm ? mk_bin_cmd_3(C_SEQ, cm, c1) : c1;
		}

	} while (**cr == ';' || **cr == '\n' || **cr == '&');

	*rt = cm;
	return cm ? 0 : 1;
}
