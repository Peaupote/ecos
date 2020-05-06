#ifndef _SH_H
#define _SH_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

enum keywords {
	// KW provoquant une descente
	KW_group_bg, // {
	KW_while,
	KW_if,
	// KW provoquant une montée
	KW_group_ed, // }
	KW_do,
	KW_done,
	KW_then,
	KW_elif,
	KW_else,
	KW_fi
};

enum redir_ty {
	IN_DUP,  IN_FILE,
	OUT_DUP, OUT_TRUNC, OUT_APPEND
};
typedef struct {
	int           fd;
	enum redir_ty type;
	union {
		char*     filewd;
		int       src_fd;
	};
} redir_t;

struct cmd_3;

enum cmd_0_ty {
	C_BAS, C_SUB
};
typedef struct {
	enum cmd_0_ty ty;
	union {
		char*         bas;
		struct cmd_3* sub; //may be NULL
	};
} cmd_0_t;

typedef struct {
	size_t   redc;
	redir_t* reds;
} red_list_t;

typedef struct {
	cmd_0_t    c;
	red_list_t r;
} cmd_1_t;

typedef struct {
	size_t   cmdc;
	cmd_1_t* cmds;
} cmd_2_t;

enum cmd_3_ty {
	C_CM2, C_SEQ, C_AND, C_OR, C_BG, C_RED, C_WHL, C_IF
};
typedef struct cmd_3{
	enum cmd_3_ty ty;
	uint8_t       sibnum;
	struct cmd_3* parent;
	union {
		cmd_2_t cm2;
		struct cmd_3* childs[2]; // != NULL
		struct {
			struct cmd_3* c; // may be NULL
			red_list_t    r;
		} red;
		struct {
			struct cmd_3* cnd; // may be NULL
			struct cmd_3* bdy; // may be NULL
		} whl;
		struct {
			struct cmd_3* cnd;    // may be NULL
			struct cmd_3* brs[2]; // may be NULL
		} cif;
	};
} cmd_3_t;

typedef struct ecmd_stack_red {
	struct ecmd_stack_red* up;
	int     sv_fds[3];
	size_t  nb_files;
	int     files[];
} ecmd_stack_red_t;

typedef struct ecmd_stack {
	struct ecmd_stack* up;
	union {
		ecmd_stack_red_t red;
		int     loop_last_st;
	};
} ecmd_stack_t;
typedef struct {
	ecmd_stack_t*     stack;
	ecmd_stack_red_t* reds;
	int               fds[3];
} ecmd_st;

typedef struct {
	pid_t pid;
	int st;
} ecmdp_t;
typedef struct ecmd_2 {
	int            num;
	ecmd_st*       st;
	const cmd_3_t *c;// C_CM2
	struct ecmd_2 *next;
	int            nb_alive;
	ecmdp_t        procs[];
} ecmd_2_t;

extern ecmd_2_t* ecmd_llist;
extern int       next_cmd_num;
extern bool      update_cwd;

typedef struct var {
	char* name;
	char* val;
	struct var* next;
} var_t;

typedef bool (*builtin_top)(int argc, char** args, int* st);
typedef int  (*builtin_syf)(int argc, char** args, int fdin);
typedef int  (*builtin_f  )(int argc, char** args);
enum builtin_ty {
	BLTI_TOP, BLTI_SYNC, BLTI_ASYNC
};
typedef struct builtin {
	char*           name;
	enum builtin_ty ty;
	bool            exp;
	union {
		builtin_f       fun;
		builtin_syf     sfun;
		builtin_top     tfun;
	};
	struct builtin* next;
} builtin_t;

#define GEN_BUF(T, N) \
typedef T N##_data_t; \
typedef struct {\
	T*  c;\
	size_t sz;\
	size_t sp;\
} N##_t;\
static inline void N##_init(N##_t* b, size_t sp) {\
	b->c  = malloc((b->sp = sp) * sizeof(T));\
	b->sz = 0;\
}\
static inline T* N##_mk(N##_t* b) {\
	if (b->sz >= b->sp) {\
		b->sp  = b->sp * 2 + 1;\
		b->c   = realloc(b->c, b->sp * sizeof(T));\
	}\
	return b->c + b->sz++;\
}\
static inline T* N##_mkn(N##_t* b, size_t n) {\
	if (b->sz + n > b->sp) {\
		do {\
			b->sp = b->sp * 2 + 1; \
		} while (b->sz + n > b->sp);\
		b->c = realloc(b->c, b->sp * sizeof(T));\
	}\
	T* rt  = b->c + b->sz; \
	b->sz += n; \
	return rt;\
}\
static inline void N##_put(N##_t* b, T c) {\
	if (b->sz >= b->sp) {\
		b->sp = b->sp * 2 + 1;\
		b->c   = realloc(b->c, b->sp * sizeof(T));\
	}\
	b->c[b->sz++] = c;\
}\
static inline void N##_putn(N##_t* b, const N##_data_t* c, size_t n) {\
	if (b->sz + n > b->sp) {\
		do {\
			b->sp = b->sp * 2 + 1; \
		} while (b->sz + n > b->sp);\
		b->c = realloc(b->c, b->sp * sizeof(T));\
	}\
	for (size_t i = 0; i < n; ++i)\
		b->c[b->sz++] = c[i];\
}\
static inline T* N##_shrink(N##_t* b) {\
	return b->c = realloc(b->c, (b->sp = b->sz) * sizeof(T));\
}\
static inline void N##_destr(N##_t* b) {\
	free(b->c);\
	b->c  = NULL;\
	b->sz = b->sp = 0;\
}
GEN_BUF(char,     cbuf)
GEN_BUF(char*,    pbuf)
GEN_BUF(redir_t, redbuf)
#undef GEN_BUF

enum src_peek_ty {
	PEEK_W0 = 0, PEEK_WDR, PEEK_KW
};
typedef struct {
	bool is_str;
	union {
		const char* ssrc;
		FILE*       fsrc;
	};

	int    err;
	size_t pos_l;
	size_t pos_c;

	size_t pk2;
	char   peek2[2];
	enum src_peek_ty pkty;
	char*  peekw;// si PEEK_KW: may be NULL
	int    keyw;
} src_t;

void destr_red_list(red_list_t* l);
void destr_redir(redir_t* rd);
void destr_cmd_0(cmd_0_t* ptr);
void destr_cmd_1(cmd_1_t* ptr);
void destr_cmd_2(cmd_2_t* ptr);
void destr_cmd_3(cmd_3_t* c);
void destr_cmd_3nl(cmd_3_t** c);

const cmd_3_t* cmd_top(const cmd_3_t* c);
void pp_cmd_0(FILE* f, const cmd_0_t* c);
void pp_redir(FILE* f, const redir_t* r);
void pp_red_list(FILE* f, const red_list_t* r);
void pp_cmd_1(FILE* f, const cmd_1_t* c);
void pp_cmd_2(FILE* f, const cmd_2_t* c);
void pp_cmd_3(FILE* f, char lvl, const cmd_3_t* c);

enum pctype {
	// terminent les keywords
	CT_COM, CT_SPA, CT_RED, CT_CNJ, 
	// ne terminent pas les keywords
	CT_ESC, CT_VAR, CT_NRM
};
enum pctype getctype(char c);
void init_parse();
void mk_ssrc(src_t* rt, const char* src);
void mk_fsrc(src_t* rt, FILE* src);
void destr_src(src_t* src);
/**
 * Renvoi 0 si un objet a été construit
 * Renvoi 1 si aucun objet n'a été construit (pour cmd_3, *rt = NULL)
 * Renvoi 2 en cas d'erreur (pour cmd_3, *rt = NULL)
 * */
int parse_cmd_0 (src_t* s, cmd_0_t* rt);
int parse_redir (src_t* s, redir_t* rt);
int parse_cmd_1 (src_t* s, cmd_1_t* rt);
int parse_cmd_2 (src_t* s, cmd_2_t* rt);
int parse_cmd_3a(src_t* s, cmd_3_t** rt);
int parse_cmd_3b(src_t* s, cmd_3_t** rt);
int parse_cmd_3c(src_t* s, cmd_3_t** rt);
int parse_sh    (src_t* s, cmd_3_t** rt);

void       init_builtins();
builtin_t* find_builtin(const char* name);
void       expand(const char* c, pbuf_t* wds);
var_t*     var_set(const char* name, char* val);
var_t*     find_lvar(const char* name);
char*      get_var(const char* name);
var_t*     arg_var_assign(char* arg);
ecmd_2_t*  exec_cmd_2(const cmd_2_t* c2, ecmd_st* st);
__attribute__((noreturn))
void       start_sub(const cmd_3_t* c3, bool keep_stdin);
int        exec_cmd_3_bg(const cmd_3_t* c3);
ecmd_2_t*  exec_cmd_3_down(const cmd_3_t* c3, ecmd_st* st, int* r_st);
ecmd_2_t*  exec_cmd_3_up(const cmd_3_t* c3, ecmd_st* st,
							int* r_st, int c_st);
ecmd_2_t*  continue_job(ecmd_2_t* e, int* st);
int        run_sub(ecmd_2_t* e);
bool       run_fg(int* st);
ecmd_2_t** find_exd_num(int num);
ecmd_st*   mk_ecmd_st();

#endif
