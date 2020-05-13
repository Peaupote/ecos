#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <util/misc.h>

#include <headers/tty.h>

#include "fxm.h"
#include "graph.h"

#define MAX_PLOT_SUB 500
#define MAX_PLOT_FUN 10
#define MAX_PLOT3_SUB 100
#define ERRL __LINE__
#define ERR_MISSING 1
#define ERR_TYPE    2
#define ERR_DOMAIN  3
#define ERR_NDEF    4

enum item_ty {
    I_NB, I_STR
};
typedef struct item {
    enum item_ty ty;
    union {
        fxm_t nb;
        char* str;
    };
} item_t;

typedef struct stack {
    item_t* c;
    size_t  sp;
    size_t  sz;
    int     err;
} stack_t;

typedef bool (*op_t)(stack_t* stk);

enum citem_ty {
    C_NB, C_OP, C_STR, C_FUN, C_VAR
};
struct fun_def;
typedef struct citem {
    struct citem* next;
    enum citem_ty ty;
    union {
        fxm_t           nb;
        const char*     str;
        op_t            op;
        const char*     var;
        struct fun_def* fun;
    };
} citem_t;

typedef struct fun_def {
    struct fun_def* next;
    const char* name;
    bool   is_op;
    union {
        op_t     op;
        citem_t* cmd;
    };
} fun_def_t;

void free_cmd(citem_t* c) {
    while (c) {
        citem_t* n = c->next;
        if (c->ty == C_STR)
            free((char*)c->str);
        else if (c->ty == C_VAR)
            free((char*)c->var);
        free(c);
        c = n;
    }
}
void destr_cmd(citem_t* c) {
    if (c->ty == C_STR)
        free((char*)c->str);
    else if (c->ty == C_VAR)
        free((char*)c->var);
    free_cmd(c->next);
}
void destr_item(item_t* i) {
    if (i->ty == I_STR) {
        free(i->str);
        i->ty = I_NB;
    }
}

// --Fonctions--

fun_def_t* fun_def_ht[256];

void set_fun_def(fun_def_t* def) {// cmd est déplacée
    unsigned char h = hash_str8(def->name, 0);
    for (fun_def_t* it = fun_def_ht[h]; it; it = it->next)
        // On ne change pas l'adresse afin que les références restent
        // valides
        if (!strcmp(def->name, it->name)) {
            if (!it->is_op) free_cmd(it->cmd);
            if ((it->is_op = def->is_op))
                it->op = def->op;
            else
                it->cmd = def->cmd;
            return;
        }

    fun_def_t*  d = malloc(sizeof(fun_def_t));
    memcpy(d, def, sizeof(fun_def_t));
    d->name       = strdup(def->name);
    d->next       = fun_def_ht[h];
    fun_def_ht[h] = d;
}
fun_def_t* find_fun_def(const char* name) {
    unsigned char h = hash_str8(name, 0);
    for (fun_def_t* it = fun_def_ht[h]; it; it = it->next)
        if (!strcmp(name, it->name))
            return it;
    return NULL;
}

// --Stack--

static inline bool pop_nb(stack_t* stk, fxm_t* nb) {
    if (!stk->sz) {
        stk->err = ERR_MISSING;
        return false;
    }
    if (stk->c[stk->sz - 1].ty != I_NB) {
        stk->err = ERR_TYPE;
        return false;
    }
    *nb = stk->c[--stk->sz].nb;
    return true;
}
static inline bool nb_to_size_t(stack_t* stk, fxm_t n0, size_t* rt) {
    if (n0 & FXM_DIGITS_MASK || (n0 >>= FXM_DIGITS, n0 < 0)) {
        stk->err = ERR_DOMAIN;
        return false;
    }
    *rt = n0;
    return true;
}
static inline bool pop_sz(stack_t* stk, size_t* rt) {
    fxm_t n0;
    if (!pop_nb(stk, &n0)) return false;
    return nb_to_size_t(stk, n0, rt);
}
static inline bool pop_int(stack_t* stk, int* rt) {
    fxm_t n0;
    if (!pop_nb(stk, &n0)) return false;
    if (n0 & FXM_DIGITS_MASK) {
        stk->err = ERR_DOMAIN;
        return false;
    }
    *rt = n0 >> FXM_DIGITS;
    return true;
}
static inline bool pop_str(stack_t* stk, char** str) {
    if (!stk->sz) {
        stk->err = ERR_MISSING;
        return false;
    }
    if (stk->c[stk->sz - 1].ty != I_STR) {
        stk->err = ERR_TYPE;
        return false;
    }
    *str = stk->c[--stk->sz].str;
    return true;
}
static inline bool pop_any(stack_t* stk, item_t* rt) {
    if (!stk->sz) {
        stk->err = ERR_MISSING;
        return false;
    }
    memcpy(rt, stk->c + --stk->sz, sizeof(item_t));
    return true;
}

static inline void push_nb(stack_t* stk, fxm_t nb) {
    if (stk->sz == stk->sp) {
        stk->sp = stk->sp * 2 + 1;
        stk->c = realloc(stk->c, stk->sp * sizeof(item_t));
    }
    stk->c[stk->sz].ty = I_NB;
    stk->c[stk->sz++].nb = nb;
}
static inline void push_str(stack_t* stk, char* str) {
    if (stk->sz == stk->sp) {
        stk->sp = stk->sp * 2 + 1;
        stk->c = realloc(stk->c, stk->sp * sizeof(item_t));
    }
    stk->c[stk->sz].ty    = I_STR;
    stk->c[stk->sz++].str = str;
}

static inline void mk_stk(stack_t* stk) {
    stk->c   = malloc(8 * sizeof(item_t));
    stk->sp  = 8;
    stk->sz  = 0;
    stk->err = 0;
}

static inline void clear_stk(stack_t* stk) {
    for (size_t i = 0; i < stk->sz; ++i)
        destr_item(stk->c + i);
    stk->sz = 0;
}

// --Évaluation--

bool exec_cmd(citem_t* c, stack_t* stk);

static inline bool exec_fun(fun_def_t* f, stack_t* stk) {
    return f->is_op ? f->op(stk) : exec_cmd(f->cmd, stk);
}

bool exec_cmd(citem_t* c, stack_t* stk) {
    for (;c; c = c->next) {
        switch (c->ty) {
            case C_NB:
                push_nb(stk, c->nb);
                continue;
            case C_STR:
                push_str(stk, strdup(c->str));
                continue;
            case C_OP:
                if (!(c->op(stk))) return false;
                continue;
            case C_VAR: {
                fun_def_t* f = find_fun_def(c->var);
                if (!f) {
                    stk->err = ERR_NDEF;
                    return false;
                }
                c->ty = C_FUN;
                c->fun = f;
            }
            // FALLTHRU
            case C_FUN:
                if (!exec_fun(c->fun, stk)) return false;
                continue;
        }
    }
    return true;
}

citem_t* exec_from_item(const item_t* i) {
    switch(i->ty) {
        case I_NB: {
            citem_t* rt = malloc(sizeof(citem_t));
            rt->next = NULL;
            rt->ty   = C_NB;
            rt->nb   = i->nb;
            return rt;
            }
        case I_STR: {
            fun_def_t* f = find_fun_def(i->str);
            if (!f) return NULL;
            citem_t* rt = malloc(sizeof(citem_t));
            rt->next = NULL;
            rt->ty   = C_FUN;
            rt->fun  = f;
            return rt;
        }
    }
    return NULL;
}

// --Opérateurs--

#define BOP fxm_t a, b; if (!pop_nb(s, &b) || !pop_nb(s, &a)) return false;
#define UOP fxm_t a; if (!pop_nb(s, &a)) return false;
bool op_add(stack_t* s) {
    BOP push_nb(s, a + b);
    return true;
}
bool op_sub(stack_t* s) {
    BOP push_nb(s, a - b);
    return true;
}
bool op_mul(stack_t* s) {
    BOP push_nb(s, fxm_mul(a, b));
    return true;
}
bool op_div(stack_t* s) {
    BOP
    if (b == 0) {
        s->err = ERR_DOMAIN;
        return false;
    }
    push_nb(s, fxm_div(a, b));
    return true;
}
bool op_usdiv(stack_t* s) {
    BOP push_nb(s, fxm_div(a, b));
    return true;
}
bool op_abs(stack_t* s) {
    UOP push_nb(s, a >= 0 ? a : -a);
    return true;
}
bool op_exp(stack_t* s) {
    UOP push_nb(s, fxm_exp(a));
    return true;
}
bool op_ln(stack_t* s) {
    UOP
    if (a <= 0) {
        s->err = ERR_DOMAIN;
        return false;
    }
    push_nb(s, fxm_ln(a));
    return true;
}
bool op_sqrt(stack_t* s) {
    UOP
	if (a < 0) return false;
	push_nb(s, fxm_sqrt(a));
    return true;
}
bool op_pi(stack_t* s) {
	push_nb(s, FXM_PI);
	return true;
}
bool op_cos(stack_t* s) {
	UOP push_nb(s, fxm_cos(a));
	return true;
}
bool op_sin(stack_t* s) {
	UOP push_nb(s, fxm_sin(a));
	return true;
}
#undef UOP
#undef BOP

bool op_pow(stack_t* s) {
    fxm_t x;
    int i;
    if (!pop_int(s, &i) || !pop_nb(s, &x)) return false;
	if (i >= 0) push_nb(s, fxm_pow(x, i));
	else {
		fxm_t inv = fxm_pow(x, -i);
		if (!inv) {
			s->err = ERR_DOMAIN;
			return false;
		}
		push_nb(s, fxm_div(fxm_one(), inv));
	}
    return true;
}

bool op_dup(stack_t* s) {
    size_t n, m;
    if (!pop_sz(s, &m) || !pop_sz(s, &n)) return false;
    if (n > s->sz || m > s->sz || n + m > s->sz) {
        s->err = ERR_MISSING;
        return false;
    }
    item_t* tmp = malloc(n * sizeof(item_t));
    for (size_t i = 0; i < n; ++i) {
        item_t* src = s->c + s->sz - n - m + i;
        memcpy(tmp + i, src, sizeof(item_t));
        src->ty = I_NB;
    }
    for (size_t j = 0; j < m; ++j) {
        size_t i;
        if (s->c[s->sz - m + j].ty != I_NB
                || !nb_to_size_t(s, s->c[s->sz - m + j].nb, &i)
                || i >= n) {
            free(tmp);
            return false;
        }
        item_t* dst = s->c + s->sz - m - n + j;
        destr_item(dst);
        if ((dst->ty = tmp[i].ty) == I_STR)
            dst->str = strdup(tmp[i].str);
        else
            dst->nb = tmp[i].nb;
    }
    for (size_t i = 0; i < n; ++i)
        destr_item(tmp + i);
    free(tmp);
    for (size_t i = s->sz - n; i < s->sz; ++i)
        destr_item(s->c + i);
    s->sz -= n;
    return true;
}

struct fun_plot_p {
    citem_t* f;
    rgbcolor_t color;
    size_t sub;
};

static int graph_fd = -1;
static graph_buf_t* graph_g = NULL;

void plot_sighdl(int signum) {
	int sv_errno = errno;
	if (signum == SIGTSTP) {
		if (graph_fd >= 0)
			graph_nacq_display(graph_fd);
		kill(getpid(), SIGSTOP);
	} else if (signum == SIGCONT) {
		if (graph_fd >= 0 && graph_g) {
			graph_acq_display(graph_fd);
    		graph_display(graph_fd, graph_g);
		}
	}
	errno = sv_errno;
}

bool op_plot(stack_t* s) {
    struct fun_plot_p funs[MAX_PLOT_FUN] = {0};
    size_t nb_funs = 1;
    fxm_t xmin = 0,
          xmax = fxm_one(),
          ymin = 0,
          ymax = fxm_one();

    char* fmt = NULL;
    if (!pop_str(s, &fmt)) return false;
    size_t fmtlen = strlen(fmt);

    struct fun_plot_p* f = funs;
    f->color = 0xffffff;
    f->sub   = 50;
    for (size_t i = fmtlen; i;) {--i;
        switch (fmt[i]) {
            case 'f': {
                if (f->f) {
                    s->err = ERRL;
                    goto err0;
                }
                item_t fi;
                if (!pop_any(s, &fi)) goto err0;
                if (!(f->f = exec_from_item(&fi))) {
                    s->err = ERRL;
                    destr_item(&fi);
                    goto err0;
                }
                destr_item(&fi);
                break;
            }
            case 'c': {
                char* cstr = NULL;
                if (!pop_str(s, &cstr)) goto err0;
                if (sscanf(cstr, "%x", &f->color) != 1) {
                    free(cstr);
                    s->err = ERRL;
                    goto err0;
                }
                f->color &= 0xffffff;
                free(cstr);
            } break;
            case 's':
                if (!pop_sz(s, &f->sub)) goto err0;
                if (!f->sub) {
                    s->err = ERRL;
                    goto err0;
                }
                mina_size_t(&f->sub, MAX_PLOT_SUB);
                break;
            case 'x':
                if (!pop_nb(s, &xmin)) goto err0;
                break;
            case 'X':
                if (!pop_nb(s, &xmax)) goto err0;
                break;
            case 'y':
                if (!pop_nb(s, &ymin)) goto err0;
                break;
            case 'Y':
                if (!pop_nb(s, &ymax)) goto err0;
                break;
            case ';': {
                if (!f->f) break;
                if (nb_funs >= MAX_PLOT_FUN) {
                    s->err = ERRL;
                    goto err0;
                }
                struct fun_plot_p* nf = funs + nb_funs++;
                nf->sub   = f->sub;
                nf->color = f->color;
                f = nf;
                break;
            }
            default:
                s->err = ERRL;
                goto err0;
        }
    }
    free(fmt);
    fmt = NULL;
    if (!f->f) --nb_funs;
    if (xmax == xmin || ymax == ymin) {
        s->err = ERRL;
        goto err0;
    }

    graph_fd = graph_open_display();
    if (graph_fd < 0) goto err1;
    struct display_info di;
    if (!graph_read_infos(graph_fd, &di)) goto err2;

    unsigned d = min_unsigned(di.width, di.height);
    graph_g = graph_make_buf(
            (di.width  - d) / 2,
            (di.height - d) / 2,
            d, d);
	graph_clear(graph_g);

    stack_t stk;
    mk_stk(&stk);
    unsigned pts[MAX_PLOT_SUB + 1];
    bool   valid[MAX_PLOT_SUB + 1];

    int      axis_x_pos = fxm_to_int(fxm_div(ymax,  ymax - ymin) * (d-1)),
             axis_y_pos = fxm_to_int(fxm_div(-xmin, xmax - xmin) * (d-1));
    if (0 <= axis_x_pos && (unsigned)axis_x_pos < d)
        graph_draw_line(graph_g,
                0,   axis_x_pos,
                d-1, axis_x_pos,
                0x777777);
    if (0 <= axis_y_pos && (unsigned)axis_y_pos < d)
        graph_draw_line(graph_g,
                axis_y_pos, 0,
                axis_y_pos, d-1,
                0x777777);

    for (size_t i = 0; i < nb_funs; ++i) {
        struct fun_plot_p* f = funs + i;

        for (unsigned i = 0; i <= f->sub; ++i) {
            push_nb(&stk, xmin + ((xmax - xmin) * (int64_t)i) / f->sub);
            valid[i] = exec_cmd(f->f, &stk);
            fxm_t v = 0;
            if (!pop_nb(&stk, &v) || v < ymin || v > ymax) {
                valid[i] = false;
                clear_stk(&stk);
                continue;
            }
            pts[i] = fxm_to_int(fxm_div(ymax - v, ymax - ymin) * (d - 1));
            valid[i] &= pts[i] < d;
            clear_stk(&stk);
        }
        free_cmd(f->f);
        f->f = NULL;

        for (unsigned i = 0; i < f->sub; ++i)
            if (valid[i] && valid[i+1]) {
                graph_draw_line(graph_g,
                        i * (d-1) / f->sub, pts[i],
                        (i+1) * (d-1) / f->sub, pts[i + 1],
                        f->color);
            }

    }
    free(stk.c);

	sighandler_t prev_sigtstp = signal(SIGTSTP, plot_sighdl),
	             prev_sigcont = signal(SIGCONT, plot_sighdl);

    if (!graph_acq_display(graph_fd))      goto err3;
    if (!graph_display(graph_fd, graph_g)) goto err3;

    int c;
    while ((c = fgetc(stdin)) != EOF && c != '\n');
	
	signal(SIGTSTP, prev_sigtstp);
	signal(SIGCONT, prev_sigcont);

    free(graph_g);
	graph_g = NULL;
    close(graph_fd);
	graph_fd = -1;
    return true;

err3:
	signal(SIGTSTP, prev_sigtstp);
	signal(SIGCONT, prev_sigcont);
    free(graph_g);
	graph_g = NULL;
err2:
    close(graph_fd);
	graph_fd = -1;
err1:
    s->err = ERRL;
err0:
    for (size_t i = 0; i < nb_funs; ++i)
        free_cmd(funs[i].f);
    free(fmt);
    return false;
}

bool get_live(tty_live_t* rt) {
	int c;
	while ((c = fgetc(stdin)) != TTY_LIVE_MAGIC)
		if (c == EOF) return false;
	char* buf = (char*)rt;
	buf[0] = TTY_LIVE_MAGIC;
	for (size_t i = 1; i < sizeof(tty_live_t); ++i) {
		if ((c = fgetc(stdin)) == EOF) return false;
		buf[i] = c;
	}
	return true;
}

static bool plot3_redraw_part = false;

void plot3_sighdl(int signum) {
	int sv_errno = errno;
	if (signum == SIGTSTP) {
		if (graph_fd >= 0)
			graph_nacq_display(graph_fd);
		write(STDOUT_FILENO, "\033d", 2);
		kill(getpid(), SIGSTOP);
	} else if (signum == SIGCONT) {
		write(STDOUT_FILENO, "\033l", 2);
		if (graph_fd >= 0 && graph_g && !plot3_redraw_part) {
			graph_acq_display(graph_fd);
    		graph_display(graph_fd, graph_g);
		}
	}
	errno = sv_errno;
}

static inline void plot3_apply_rot(fxm_t rot[], unsigned axis, bool posi) {
	fxm_t rot_v[3] = {0, 0, 0};
	rot_v[axis] = posi ? FXM_PI / 16 : - FXM_PI / 16;
	fxm_t arot[16];
	fxm_t tmp [16];
	fxm_mrot(arot, rot_v);
	fxm_mmul(arot, rot, tmp, 4, 4, 4);
	memcpy(rot, tmp, sizeof(tmp));
}

bool op_plot3(stack_t* s) {
	fxm_t vals[MAX_PLOT3_SUB + 1][MAX_PLOT3_SUB + 1];
	bool valid[MAX_PLOT3_SUB + 1][MAX_PLOT3_SUB + 1];
    citem_t* f = NULL;
	unsigned sub = 50;

	{
		size_t s_sub;
		if (!pop_sz(s, &s_sub)) goto err0;
		if (s_sub)
			sub = min_size_t(MAX_PLOT3_SUB, s_sub);

		item_t fi;
		if (!pop_any(s, &fi)) goto err0;
		if (!(f = exec_from_item(&fi))) {
			s->err = ERRL;
			destr_item(&fi);
			goto err0;
		}
		destr_item(&fi);
	}
	{
		stack_t stk;
		mk_stk(&stk);
		for (unsigned y = 0; y <= sub; ++y)
			for (unsigned x = 0; x <= sub; ++x) {
				push_nb(&stk, fxm_of_int(x) / sub);
				push_nb(&stk, fxm_of_int(y) / sub);
				valid[y][x] = false;
				valid[y][x] = exec_cmd(f, &stk)
						    && pop_nb(&stk, vals[y] + x);
				clear_stk(&stk);
			}
		free(stk.c);
	}
	free_cmd(f);

    graph_fd = graph_open_display();
    if (graph_fd < 0) goto err1;
    struct display_info di;
    if (!graph_read_infos(graph_fd, &di)) goto err2;

    graph_g = graph_make_buf(0, 0, di.width, di.height);

	fxm_t proj[16],
	      tr  [16],
	      rot [16],
		  tr0 [16],
		  m_w [16],
	      m_pw[16];
	fxm_t zoom = fxm_one();

	fxm_t tr0_v[3] = {-fxm_one()/2, -fxm_one()/2, -fxm_one()/2};
	fxm_mtrl(tr0, tr0_v);
	fxm_mid(rot, 4);
	fxm_t tr_v[3] = {0, fxm_of_int(2), 0};

	plot3_redraw_part = true;
	
	sighandler_t prev_sigtstp = signal(SIGTSTP, plot3_sighdl),
	             prev_sigcont = signal(SIGCONT, plot3_sighdl);

	write(STDOUT_FILENO,"\033l", 2);
redraw:
	plot3_redraw_part = true;
	{
		fxm_mtrl(tr,   tr_v);
		fxm_t m_w0[16];
		fxm_mmul(rot,  tr0,  m_w0, 4, 4, 4);
		fxm_mmul(tr,   m_w0, m_w,  4, 4, 4);
		fxm_mproj_h(proj, di.width, di.height, zoom, 0, fxm_of_int(4));
		fxm_mmul(proj, m_w,  m_pw, 4, 4, 4);
	}

	graph_clear3(graph_g);

	fxm_t points[16];
	for (unsigned k = 0; k < 4; ++k)
		points[12 + k] = fxm_one();
	const unsigned tri_index[2][3] = {{0, 1, 2}, {3, 2, 1}};

	for (unsigned y = 0; y < sub; ++y)
		for (unsigned x = 0; x < sub; ++x) {
			if (!valid[y+1][x] || !valid[y][x+1])
				continue;
			for (unsigned dy = 0; dy < 2; ++dy)
				for (unsigned dx = 0; dx < 2; ++dx) {
					points[  2*dy + dx  ] = fxm_of_int(x + dx) / sub;
					points[4 + 2*dy + dx] = fxm_of_int(y + dy) / sub;
					points[8 + 2*dy + dx] = vals[y + dy][x + dx];
				}
			fxm_t spos[16];
			fxm_t wpos[16];
			fxm_mmul(m_pw, points, spos, 4, 4, 4);
			fxm_mmul(m_w,  points, wpos, 4, 4, 4);

			for (unsigned k = 0; k < 2; ++k) {
				const unsigned* tri = tri_index[k];
				if (!valid[y + (tri[1]>>1)][x + (tri[0] &1)])
					continue;
				int x[3], y[3];
				fxm_t depth[3];
				for (unsigned i = 0; i < 3; ++i) {
					x[i] = fxm_to_int(spos[  tri[i]  ]);
					y[i] = fxm_to_int(spos[4 + tri[i]]);
					depth[i] = spos[8 + tri[i]];
				}
				fxm_t ssegs[2][2];
				for (unsigned i = 0; i < 2; ++i) {
					ssegs[0][i] = spos[4*i + tri[1]]
						        - spos[4*i + tri[0]];
					ssegs[1][i] = spos[4*i + tri[2]]
						        - spos[4*i + tri[0]];
				}
				bool back_face = fxm_mul(ssegs[0][0],ssegs[1][1]) 
							   > fxm_mul(ssegs[1][0], ssegs[0][1]);
				rgbcolor_t color = back_face ? 0x00232c : 0x601414;
				fxm_t wsegs[2][3];
				fxm_t normal0[3], normal[3] = {0};
				for (unsigned i = 0; i < 3; ++i) {
					wsegs[0][i] = wpos[4*i + tri[1]]
								- wpos[4*i + tri[0]];
					wsegs[1][i] = wpos[4*i + tri[2]]
								- wpos[4*i + tri[0]];
				}
				fxm_vprod(wsegs[0], wsegs[1], normal0);
				fxm_normalize(normal0, normal, 3);
				fxm_t dot = normal[0] - normal[1] + normal[2];
				if (back_face) dot = -dot;
				if (dot > 0) {
					rgbcolor_t add = (dot * 0x20) & 0xff0000;
					color += add | (add >> 8) | (add >> 16);
				}
				
				graph_draw_tri(graph_g, x, y, depth, color);
			}
		}

    if (!graph_acq_display(graph_fd)) goto err3;
    if (!graph_display(graph_fd, graph_g))  goto err3;
    
	plot3_redraw_part = false;

	tty_live_t lt;
    while (get_live(&lt)) {
		if (lt.ev.key == KEY_ESCAPE) break;
		if (lt.ev.flags & KEY_FLAG_PRESSED) {
			switch (lt.ev.flags & KEY_FLAG_MODS) {
				case 0: switch (lt.ev.key) {
					case KEY_LEFT_ARROW:
						tr_v[0] -= fxm_div(fxm_one(), zoom) / 4;
						goto redraw;
					case KEY_RIGHT_ARROW:
						tr_v[0] += fxm_div(fxm_one(), zoom) / 4;
						goto redraw;
					case KEY_UP_ARROW:
						tr_v[2] += fxm_div(fxm_one(), zoom) / 4;
						goto redraw;
					case KEY_DOWN_ARROW:
						tr_v[2] -= fxm_div(fxm_one(), zoom) / 4;
						goto redraw;
				} break;
				case KEY_FLAG_CTRL:{
					switch (lt.ev.key) {
					case KEY_LEFT_ARROW:
						plot3_apply_rot(rot, 2, true);
						goto redraw;
					case KEY_RIGHT_ARROW:
						plot3_apply_rot(rot, 2, false);
						goto redraw;
					case KEY_UP_ARROW:
						plot3_apply_rot(rot, 0, true);
						goto redraw;
					case KEY_DOWN_ARROW:
						plot3_apply_rot(rot, 0, false);
						goto redraw;
					}
				} break;
				case KEY_FLAG_SHIFT:{
					switch (lt.ev.key) {
					case KEY_LEFT_ARROW:
						plot3_apply_rot(rot, 1, false);
						goto redraw;
					case KEY_RIGHT_ARROW:
						plot3_apply_rot(rot, 1, true);
						goto redraw;
					case KEY_UP_ARROW:
						zoom = fxm_mul(zoom, fxm_of_int(9)/8);
						goto redraw;
					case KEY_DOWN_ARROW:
						zoom = fxm_mul(zoom, fxm_of_int(8)/9);
						goto redraw;
					}
				} break;
			}
		}
	}

	signal(SIGTSTP, prev_sigtstp);
	signal(SIGCONT, prev_sigcont);

	free(graph_g);
	graph_g  = NULL;
    close(graph_fd);
	graph_fd = -1;
	write(STDOUT_FILENO,"\033d", 2);
	return true;

err3:
	signal(SIGTSTP, prev_sigtstp);
	signal(SIGCONT, prev_sigcont);
    free(graph_g);
	graph_g  = NULL;
err2:
    close(graph_fd);
	graph_fd = -1;
err1:
    s->err   = ERRL;
err0:
	write(STDOUT_FILENO,"\033d", 2);
    return false;
}

#define DEF_OP(N) {char buf[]=#N;fun_def_t def = {.next=0,.name=buf,\
                    .is_op=true,.op=op_##N};set_fun_def(&def);}
void init_funs() {
    DEF_OP(abs)
    DEF_OP(usdiv)
    DEF_OP(pow)
    DEF_OP(exp)
    DEF_OP(ln)
    DEF_OP(sqrt)
    DEF_OP(pi)
    DEF_OP(cos)
    DEF_OP(sin)
    DEF_OP(dup)
    DEF_OP(plot)
    DEF_OP(plot3)
}
#undef DEF_OP

// --Lecture--

static int get_nblank(FILE* f) {
    int c;
    while (isblank(c = fgetc(f)));
    if (c == '#') while ((c = fgetc(f)) != EOF && c != '\n');
    return c;
}

static inline bool isvname0(int c) {
    return isalpha(c) || c == '_';
}
static inline bool isvname(int c) {
    return isalnum(c) || c == '_';
}
static inline bool read_var(FILE* f, char* buf) {
    for (size_t i = 0; i < 256; ++i) {
        int c = fgetc(f);
        if (c == EOF) {
            buf[i] = '\0';
            return true;
        } else if (!isvname(c)) {
            buf[i] = '\0';
            ungetc(c, f);
            return true;
        } else buf[i] = c;
    }
    return false;
}

citem_t* read_nb_item(FILE* f, bool neg) {
    fxm_t n = 0;
    if (!fxm_scanu(f, &n)) return NULL;
    citem_t* rt = malloc(sizeof(citem_t));
    rt->ty = C_NB;
    rt->nb = neg ? -n : n;
    return rt;
}
citem_t* mk_blti_item(bool (*fun)(stack_t*)) {
    citem_t* rt = malloc(sizeof(citem_t));
    rt->ty = C_OP;
    rt->op = fun;
    return rt;
}

int read_item(FILE* f, citem_t** rt) {
    int c = get_nblank(f);
    switch (c) {
        case '+': case '-': {
            int c2 = fgetc(f);
            ungetc(c2, f);
            if (isdigit(c2) || c2 == '.') {
                *rt = read_nb_item(f, c == '-');
                return 0;
            }
            *rt = mk_blti_item(c == '+' ? op_add : op_sub);
            return 0;
        }
        case '*':
            *rt = mk_blti_item(op_mul);
            return 0;
        case '/':
            *rt = mk_blti_item(op_div);
            return 0;
        case '\'': case '"': {
            char buf[256];
            for (size_t i = 0; i < 256; ++i) {
                int c2 = fgetc(f);
                if (c2 == EOF) return 1;
                else if (c2 == c) {
                    buf[i] = '\0';
                    *rt = malloc(sizeof(citem_t));
                    (**rt).ty  = C_STR;
                    (**rt).str = strdup(buf);
                    return 0;
                }
                buf[i] = c2;
            }
            return 2;
        }
        default:
            ungetc(c, f);
            if (isvname0(c)) {
                char buf[256];
                if (!read_var(f, buf)) return 4;
                *rt = malloc(sizeof(citem_t));
                (**rt).ty  = C_VAR;
                (**rt).var = strdup(buf);
                return 0;
            }
            *rt = read_nb_item(f, false);
            return 0;
    }
}

int read_cmd(FILE* f, citem_t*** r) {// ***r doit être NULL
    int rts;
    citem_t** p = *r;
    while (!(rts = read_item(f, p))) {
        if (!*p) {
            *r = p;
            return 0;
        }
        (**p).next = NULL;
        p = &((**p).next);
    }
    *r = p;
    return rts;
}


int exec0(citem_t* c) {
    stack_t stk;
    mk_stk(&stk);

    exec_cmd(c, &stk);

    if (stk.sz != 1) printf("(");
    for (size_t i = 0; i < stk.sz; ++i) {
        if (i) printf(", ");
        switch (stk.c[i].ty) {
            case I_NB:
                fxm_print(stdout, stk.c[i].nb, 6);
                break;
            case I_STR:
                printf("%s", stk.c[i].str);
        }
    }
    if (stk.sz == 1) printf("\n");
    else printf(")\n");

    if (stk.err)
        fprintf(stderr, "error %d\n", stk.err);

    clear_stk(&stk);
    free(stk.c);
    return stk.err;
}

int run(FILE* f, bool prompt) {
    citem_t*   cmd = NULL;
    citem_t** ccmd = &cmd;
    bool       new = true;
    char       buf[256];
    while (true) {
        if (prompt) {
            printf(new ? "\033p3mat> \033;" : "\033p3.. \033;");
            fflush(stdout);
        }
        int c = get_nblank(f);
        if (c == EOF) {
            free_cmd(cmd);
            return 0;
        }

        int st;
        if (new) {
            if (c == ':') {
                if (!read_var(f, buf)){
                    st = 3;
                    goto err;
                }
            } else {
                ungetc(c, f);
                buf[0] = '\0';
            }
        } else ungetc(c, f);

        st = read_cmd(f, &ccmd);
        bool cont = false;
        if (st || ((c = fgetc(f), c != EOF)
                    && c != '\n'
                    && (c != '\\'
                        || (c = fgetc(f),cont = true, c != '\n')))) {
            if (st)
                err: fprintf(stderr, "Erreur de syntaxe (%d)\n", st);
            else
                fprintf(stderr, "caractere inattendu: '%c'\n", c);
            if (!prompt) {
                free_cmd(cmd);
                return 1;
            }
        } else if (cont) {
            new = false;
            continue;
        } else if (buf[0]) {
            fun_def_t def = {.next=0, .name=buf, .is_op=false, .cmd=cmd};
            set_fun_def(&def);
            cmd = NULL;
        } else if (cmd)
            exec0(cmd);

        free_cmd(cmd);
        new  = true;
        ccmd = &cmd;
        cmd  = NULL;
        if (c == EOF) return 0;
        continue;
    }
}

int main(int argc, char* argv[]) {
    init_funs();
    if (argc > 1) {
        for (int argi = 1; argi < argc; ++argi) {
            char* arg = argv[argi];
            if (*arg=='-' && !arg[1]) {
                int rts = run(stdin, true);
                if (rts) return rts;
            } else {
                FILE* f = fopen(arg, "r");
                if (!f) {
                    perror(arg);
                    return 2;
                }
                int rts = run(f, false);
                fclose(f);
                if (rts) return rts;
            }
        }
        return 0;
    } else
        return run(stdin, true);
}
