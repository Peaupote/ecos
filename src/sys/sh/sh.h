#ifndef _SH_H
#define _SH_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

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
		struct cmd_3* sub;
	};
} cmd_0_t;

typedef struct {
	cmd_0_t  cmd;
	size_t   redc;
	redir_t* reds;
} cmd_1_t;

typedef struct {
	size_t   cmdc;
	cmd_1_t* cmds;
} cmd_2_t;

enum cmd_3_ty {
	C_CM2, C_SEQ, C_AND, C_OR, C_BG
};
typedef struct cmd_3{
	enum cmd_3_ty ty;
	uint8_t       sibnum;
	struct cmd_3* parent;
	union {
		cmd_2_t cm2;
		struct cmd_3* childs[2];
	};
} cmd_3_t;


typedef struct {
	pid_t pid;
	int st;
} ecmdp_t;
typedef struct ecmd_2 {
	int            num;
	const cmd_3_t *c;// C_CM2
	struct ecmd_2 *next;
	int            nb_alive;
	ecmdp_t        procs[];
} ecmd_2_t;

extern ecmd_2_t* ecmd_llist;
extern int       next_cmd_num;
extern char      line[];
extern bool      update_cwd;

typedef struct var {
	char* name;
	char* val;
	struct var* next;
} var_t;

typedef bool (*builtin_top)(int argc, char** args, int* st);
typedef int  (*builtin_syf)(int argc, char** args);
typedef int  (*builtin_f  )(int argc, char** args);
enum builtin_ty {
	BLTI_TOP, BLTI_SYNC, BLTI_ASYNC
};
typedef struct builtin {
	char*           name;
	enum builtin_ty ty;
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
static inline void N##_put(N##_t* b, T c) {\
	if (b->sz >= b->sp) {\
		b->sp *= 2;\
		b->c   = realloc(b->c, b->sp * sizeof(T));\
	}\
	b->c[b->sz++] = c;\
}\
static inline void N##_putn(N##_t* b, const N##_data_t* c, size_t n) {\
	if (b->sz + n > b->sp) {\
		b->sp *= 2;\
		b->c   = realloc(b->c, b->sp * sizeof(T));\
	}\
	for (size_t i = 0; i < n; ++i)\
		b->c[b->sz++] = c[i];\
}\
static inline T* N##_shrink(N##_t* b) {\
	return b->c = realloc(b->c, (b->sp = b->sz) * sizeof(T));\
}
GEN_BUF(char,  cbuf)
GEN_BUF(char*, pbuf)
#undef GEN_BUF

void destr_redir(redir_t* rd);
void destr_cmd_0(cmd_0_t* ptr);
void destr_cmd_1(cmd_1_t* ptr);
void destr_cmd_2(cmd_2_t* ptr);
void destr_cmd_3(cmd_3_t* c);

const cmd_3_t* cmd_top(const cmd_3_t* c);
void pp_cmd_0(FILE* f, const cmd_0_t* c);
void pp_redir(FILE* f, const redir_t* r);
void pp_cmd_1(FILE* f, const cmd_1_t* c);
void pp_cmd_2(FILE* f, const cmd_2_t* c);
void pp_cmd_3(FILE* f, char lvl, const cmd_3_t* c);

enum pctype {
	CT_CNJ, CT_RED, CT_SPA, CT_NRM, CT_ESC, CT_VAR
};
enum pctype getctype(char c);
int parse_cmd_0(const char** cr, cmd_0_t* rt);
int parse_redir(const char** cr, redir_t* rt);
int parse_cmd_1(const char** cr, cmd_1_t* rt);
int parse_cmd_2(const char** cr, cmd_2_t* rt);
int parse_cmd_3a(const char** cr, cmd_3_t** rt);
int parse_cmd_3b(const char** cr, cmd_3_t** rt);
int parse_cmd_3c(const char** cr, cmd_3_t** rt);

void       init_builtins();
builtin_t* find_builtin(const char* name);
void       expand(const char* c, pbuf_t* wds);
var_t*     var_set(const char* name, char* val);
var_t*     find_lvar(const char* name);
char*      get_var(const char* name);
ecmd_2_t*  exec_cmd_2(const cmd_2_t* c2);
__attribute__((noreturn))
void       start_sub(const cmd_3_t* c3);
int        exec_cmd_3_bg(const cmd_3_t* c3);
ecmd_2_t*  exec_cmd_3_down(const cmd_3_t* c3, int* r_st);
ecmd_2_t*  exec_cmd_3_up(const cmd_3_t* c3, int* r_st, int c_st);
ecmd_2_t*  continue_job(ecmd_2_t* e, int* st);
int        run_sub(ecmd_2_t* e);
bool       run_fg(int* st);

#endif
