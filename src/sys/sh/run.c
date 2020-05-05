#include "sh.h"

#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <util/misc.h>
#include <assert.h>

// --Variables--
var_t* lvars[256] = {NULL};

var_t* var_set(const char* name, char* val) {
	unsigned char h = hash_str8(name, 0);
	for (var_t* it = lvars[h]; it; it = it->next)
		if (!strcmp(name, it->name)) {
			free(it->val);
			it->val = val;
			return it;
		}
	var_t* v = malloc(sizeof(var_t));
	v->name  = malloc(strlen(name) + 1);
	strcpy(v->name, name);
	v->val   = val;
	v->next  = lvars[h];
	return lvars[h] = v;
}
var_t* find_lvar(const char* name) {
	unsigned char h = hash_str8(name, 0);
	for (var_t* it = lvars[h]; it; it = it->next)
		if (!strcmp(name, it->name))
			return it;
	return NULL;
}
char* get_var(const char* name) {
	var_t* l = find_lvar(name);
	if (l) return l->val;
	return getenv(name);
}
var_t* arg_var_assign(char* arg) {
	char* setp = strchr(arg, '=');
	if (!setp) return NULL;
	
	char* name = arg;
	*setp      = '\0';
	char* val0 = setp + 1;
	char* val  = malloc(strlen(val0) + 1);
	strcpy(val, val0);

	return var_set(name, val);
}

// --Commandes en cours d'execution au premier plan ou stoppées--

struct ecmdp_ref {
    ecmd_2_t** cmd;
    int        num;
};
struct ecmdp_ref find_exd_proc(pid_t pid) {
    for (ecmd_2_t** cmdr = &ecmd_llist; *cmdr; cmdr = &(*cmdr)->next) {
        ecmd_2_t* cmd = *cmdr;
		size_t   cmdc = cmd->c->cm2.cmdc;
        for (size_t i = 0; i < cmdc; ++i) {
            if (cmd->procs[i].pid == pid) {
                struct ecmdp_ref rt = {cmdr, i};
                return rt;
            }
        }
    }
    struct ecmdp_ref rt = {NULL, 0};
    return rt;
}
ecmd_2_t** find_exd_num(int num) {
    for (ecmd_2_t** cmdr = &ecmd_llist; *cmdr; cmdr = &(*cmdr)->next)
        if ((*cmdr)->num == num)
            return cmdr;
    return NULL;
}
ecmd_2_t** on_child_end(pid_t pid, int st) {
    struct ecmdp_ref rf = find_exd_proc(pid);
    if (!rf.cmd) {
        printf("couldn't find %d\n", (int)pid);
        return NULL;
    }
    ecmd_2_t* cmd          = *rf.cmd;
    cmd->procs[rf.num].pid = PID_NONE;
	cmd->procs[rf.num].st  = st;
    --cmd->nb_alive;
    return rf.cmd;
}

// --Builtins--

int blti_exit(int argc, char** argv, int fd_in __attribute__((unused))) {
	int st = argc > 1 ? atoi(argv[1]) : 0;
	exit(st);
}

int blti_cd(int argc, char **argv, int fd_in __attribute__((unused))) {
	char *dst;
	if (argc == 1) {
		dst = get_var("HOME");
		if (!dst) {
			fprintf(stderr, "HOME undefined\n");
			return 1;
		}
	} else if (argc == 2)
		dst = argv[1];
	else {
        fprintf(stderr, "usage: cd [dir]\n");
        return 2;
    }
   
	update_cwd = true;
	int    rts = chdir(dst);
	if (rts) {
		perror("cd");
		return 1;
	}
	return 0;
}

int blti_export(int argc __attribute__((unused)), char **args,
		int fd_in __attribute__((unused)) ) {
	int rts = 0;
	for (char** arg = args + 1; *arg; ++arg) {
		var_t* v = arg_var_assign(*arg);
		if (!v && !(v=find_lvar(*arg))) {
			if (!rts) rts = 2;
			fprintf(stderr, "undefined: %s\n", *arg);
			continue;
		}
		if (setenv(v->name, v->val, 1)) {
			perror("export");
			if (!rts) rts = 1;
		}
	}
	return rts;
}

int blti_read(int argc, char **args, int fd_in) {
	cbuf_t buf;
	cbuf_init(&buf, 256);
	char* seg = cbuf_mkn(&buf, 256);
	int rc;
	while ((rc = read(fd_in, seg, 256)) > 0) {
		buf.sz -= 256 - rc;
		for (int i = 0; i < rc; ++i) {
			if (seg[i] == '\n') {
				lseek(fd_in, -rc+i+1, SEEK_CUR);
				seg[i] = '\0';

				char* sv;
				char* word = strtok_rnull(buf.c, " ", &sv);
				for(int argi = 1; argi < argc; ++argi) {
					if (word) {
						char* v = malloc((strlen(word) + 1) * sizeof(char));
						strcpy(v, word);
						var_set(args[argi], v);
					} else break;
					word = strtok_rnull(NULL, " ", &sv);
				}
				cbuf_destr(&buf);
				return 0;
			}
		}
		seg = cbuf_mkn(&buf, 256);
	}
	cbuf_destr(&buf);
	return 1;
}

bool blti_fg(int argc, char** args, int* st) {
	int num;
    if (argc != 2 || sscanf(args[1], "%d", &num) != 1) {
		fprintf(stderr, "usage: fg cmd_num\n");
		*st = 2;
		return true;
	}
    ecmd_2_t** er = find_exd_num(num);
    if (!er) {
        fprintf(stderr, "Aucune commande corespondante: %d\n", num);
        *st = 1;
		return true;
    }

    ecmd_2_t* e = *er;
    *er         = e->next;
    e->next     = ecmd_llist;
    ecmd_llist  = e;

	printf("[%d]\t", e->num);
	pp_cmd_3(stdout, 'c', cmd_top(e->c));
	printf("\n");

	size_t cmdc = e->c->cm2.cmdc;
    for (size_t i = 0; i < cmdc; ++i)
        if (~e->procs[i].pid && kill(e->procs[i].pid, SIGCONT))
            perror("error sending SIGCONT");

    return run_fg(st);
}

int blti_jobs(int argc __attribute__((unused)),
		char** args __attribute__((unused))) {
	
	for (ecmd_2_t* e = ecmd_llist; e; e = e->next) {
		printf("[%d]\t", e->num);
		pp_cmd_3(stdout, 'c', cmd_top(e->c));
		printf("\n");
	}

	return 0;
}

#define GEN_SBLTI(N, T, F, Y) \
static char sblti_n_##N[] = #N;\
static builtin_t sblti_##N = {.name = sblti_n_##N, .ty = T, .F = (Y)&blti_##N};
#define GEN_SBLTI_ASYNC(N) GEN_SBLTI(N,BLTI_ASYNC,fun,builtin_f)
#define GEN_SBLTI_SYNC(N) GEN_SBLTI(N,BLTI_SYNC,sfun,builtin_syf)
#define GEN_SBLTI_TOP(N) GEN_SBLTI(N,BLTI_TOP,tfun,builtin_top)
GEN_SBLTI_SYNC(exit)
GEN_SBLTI_SYNC(cd)
GEN_SBLTI_SYNC(export)
GEN_SBLTI_SYNC(read)
GEN_SBLTI_TOP(fg)
GEN_SBLTI_ASYNC(jobs)
#undef GEN_SBLTI_SYNC
#undef GEN_SBLTI_TOP
#undef GEN_SBLTI

builtin_t* builtins[256] = {NULL};

void add_builtin(builtin_t* b) {
	unsigned char h = hash_str8(b->name, 0);
	b->next         = builtins[h];
	builtins[h]     = b;
}

void init_builtins() {
	add_builtin(&sblti_exit);
	add_builtin(&sblti_cd);
	add_builtin(&sblti_export);
	add_builtin(&sblti_read);
	add_builtin(&sblti_fg);
	add_builtin(&sblti_jobs);
}

builtin_t* find_builtin(const char* name) {
	unsigned char h = hash_str8(name, 0);
	for (builtin_t* it = builtins[h]; it; it = it->next)
		if (!strcmp(name, it->name))
			return it;
	return NULL;
}

// --Gestion des interruptions--

static bool int_tstp;

void int_handler(int signum __attribute__((unused))) {
	ecmd_2_t* fgc = ecmd_llist;
	size_t   cmdc = fgc->c->cm2.cmdc;
    for (size_t i = 0; i < cmdc; ++i)
        if (~fgc->procs[i].pid && kill(fgc->procs[i].pid, SIGINT))
            perror("error sending SIGINT");
}
void tstp_handler(int signum __attribute__((unused))) {
	ecmd_2_t* fgc = ecmd_llist;
	size_t   cmdc = fgc->c->cm2.cmdc;
    for (size_t i = 0; i < cmdc; ++i)
        if (~fgc->procs[i].pid && kill(fgc->procs[i].pid, SIGTSTP))
            perror("error sending SIGSTP");
    int_tstp = true;
}

void bg_int_handler(int signum __attribute__((unused))) {
	ecmd_2_t* fgc = ecmd_llist;
	size_t   cmdc = fgc->c->cm2.cmdc;
    for (size_t i = 0; i < cmdc; ++i)
        if (~fgc->procs[i].pid && kill(fgc->procs[i].pid, SIGINT))
            perror("error sending SIGINT");
	exit(1);
}

// --Exécution--

static int red_file_flag[5] = {
	0, O_RDONLY,
	0, O_WRONLY | O_CREAT | O_TRUNC, O_WRONLY | O_CREAT | O_APPEND
};

static int redir_file_open(const redir_t* r) {
	pbuf_t buf;
	pbuf_init(&buf, 1);
	expand(r->filewd, &buf);
	if (buf.sz != 1) {
		fprintf(stderr, "|%s| = %d != 1", r->filewd, (int)buf.sz);
		for (size_t i = 0; i < buf.sz; ++i)
			free(buf.c[i]);
		pbuf_destr(&buf);
		return -1;
	}
	char* file = *buf.c;

	int ffd = open(file, red_file_flag[r->type], 0640); // TODO: umask
	if (ffd < 0) {
        char buf2[256];
		if (buf.sz > 200) file[200] = '\0';
        sprintf(buf2, "sh rdf: %s", file);
        perror(buf2);
		free(file);
		return -1;
	}
	free(file);
	return ffd;
}

static int exec_redir_file(const redir_t* r) {
	int ffd = redir_file_open(r);
	if (ffd < 0) return 1;
	if (ffd == r->fd) return 0;
	if (!~dup2(ffd, r->fd)) return 1;
	close(ffd);
	return 0;
}
static int exec_redir_dup(const redir_t* r) {
	return ~dup2(r->src_fd, r->fd) ? 0 : 1;
}
static int exec_redir(const redir_t* r) {
	switch (r->type) {
		case IN_DUP: case OUT_DUP:
			return exec_redir_dup(r);
		case IN_FILE: case OUT_TRUNC: case OUT_APPEND:
			return exec_redir_file(r);
	}
	return 2;//never reached
}


static int apply_reds3(ecmd_st* st) {
	// DFS pour repérere les interdépendances
	int sv_dup[3] = {-1, -1, -1};
	char  t[3] = {0};
	int stk[9] = {0, 1, 2};
	int stki = 3;
	while (stki) {
		int i = stk[stki - 1];
		if (t[i] == 0) {
			if (st->fds[i] == i) {
				t[i] = 2;
				--stki;
			} else {
				for (int j = 0; j < 3; ++j)
					if (st->fds[j] == i && t[j] == 1) {
						sv_dup[i] = dup(i);
						break;
					}
				if (~sv_dup[i]) {
					t[i] = 2;
					dup2(st->fds[i], i);
					--stki;
				} else {
					for (int j = 0; j < 3; ++j)
						if (st->fds[j] == i && t[j] == 0)
							stk[stki++] = j;
					t[i] = 1;
				}
			}
		} else {
			if (t[i] == 1) {
				if (st->fds[i] < 3 && ~sv_dup[st->fds[i]])
					dup2(sv_dup[st->fds[i]], i);
				else
					dup2(st->fds[i], i);
				t[i] = 2;
			}
			--stki;
		}
	}
	for (int i = 0; i < 3; ++i)
		if (~sv_dup[i])
			close(sv_dup[i]);

	// fermeture des fichiers ouverts par sh
	for (ecmd_stack_red_t* oit = st->reds; oit; oit = oit->up)
		for (size_t i = 0; i < oit->nb_files; ++i)
			close(oit->files[i]);
	return 0;
}

static int reg_reds3(ecmd_st* st, const red_list_t* rs) {
	int nb_f = 0;
	for (size_t i = 0; i < rs->redc; ++i) {
		if (rs->reds[i].fd < 0 || rs->reds[i].fd > 2) {
			fprintf(stderr, "reds3 fd=%d\n", rs->reds[i].fd);
			return 2;
		}
		switch (rs->reds[i].type) {
			case IN_FILE: case OUT_TRUNC: case OUT_APPEND:
				++nb_f;
				break;
			case IN_DUP: case OUT_DUP: break;
		}
	}

	ecmd_stack_t* sk = malloc(offsetof(ecmd_stack_t, red.files)
						+ nb_f * sizeof(int));
	for (int i = 0; i < 3; ++i)
		sk->red.sv_fds[i] = st->fds[i];

	size_t itf = 0;
	int rts    = 0;
	for (size_t i = 0; i < rs->redc; ++i) {
		switch(rs->reds[i].type) {
			case IN_FILE: case OUT_TRUNC: case OUT_APPEND:
				st->fds[rs->reds[i].fd] = sk->red.files[itf]
					= redir_file_open(rs->reds + i);
				if (sk->red.files[itf] < 0) {
					rts = 1;
					goto err;
				}
				++itf;
				break;
			case IN_DUP: case OUT_DUP:
				if (rs->reds[i].src_fd < 0 || rs->reds[i].src_fd > 2) {
					fprintf(stderr, "reds3 sfd=%d\n", rs->reds[i].src_fd);
					rts = 2;
					goto err;
				}
				st->fds[rs->reds[i].fd] = st->fds[rs->reds[i].src_fd];
				break;
		}
	}

	sk->red.nb_files = itf;
	sk->up           = st->stack;
	st->stack        = sk;
	sk->red.up       = st->reds;
	st->reds         = &sk->red;
	return 0;
err:
	for (size_t i = 0; i < itf; ++i)
		close(sk->red.files[itf]);
	free(sk);
	return rts;
}

static void unreg_reds3(ecmd_st* st) {
	ecmd_stack_t* sk = st->stack;
	for (size_t i = 0; i < sk->red.nb_files; ++i)
		close(sk->red.files[i]);
	for (int i = 0; i < 3; ++i)
		st->fds[i] = sk->red.sv_fds[i];
	st->stack = sk->up;
	st->reds  = sk->red.up;
	free(sk);
}


char* expand_find_var(const char** src) {
	*src += 2;
	const char* bg = *src;
	while(*++*src != '}');
	size_t len = *src - bg;
	char* name = malloc((len + 1) * sizeof(char));
	memcpy(name, bg, *src - bg);
	name[len] = '\0';
	char* val = get_var(name);
	free(name);
	return val;
}

void expand_var(const char** src, cbuf_t* b) {
	char* val = expand_find_var(src);
	if(val)
		cbuf_putn(b, val, strlen(val));
}

bool word_split(const char** src, pbuf_t* b0, cbuf_t* b1, bool b1w) {
	if (b1w) {
		if (getctype(**src) == CT_SPA) {
			cbuf_put(b1, '\0');
			pbuf_put(b0, cbuf_shrink(b1));
		} else if (**src)
			goto rvw;
		else return true;
	}
	while (true) {
		while (getctype(**src) == CT_SPA) ++*src;
		if (!**src) return false;
		cbuf_init(b1, 256);
rvw:;
		const char* src0 = *src;
		while (getctype(*++*src) != CT_SPA && **src);
		cbuf_putn(b1, src0, *src - src0);
		if (!**src) return true;
		cbuf_put(b1, '\0');
		pbuf_put(b0, cbuf_shrink(b1));
	}
}

bool expand_var_split(const char** src, pbuf_t* b0, cbuf_t* b1, bool b1w) {
	char* val = expand_find_var(src);
	if(!val) return b1w;
	return word_split((const char**)&val, b0, b1, b1w);
}

void expand_escpd_char(char e, const char** src, cbuf_t* b) {
	char c = *++*src;
	if (e ? c == e || c == '\\' : getctype(c) != CT_NRM)
		cbuf_put(b, c);
	else {
		char buf[2] = {'\\', c};
		cbuf_putn(b, buf, 2);
	}
}

void expand_escpd_str(const char** src, cbuf_t* b) {
	char ty = **src;
	char c;
	while ((c = *++*src) != ty) {
		if (c == '\\')
			expand_escpd_char(ty, src, b);
		else if (c == '$' && ty == '"')
			expand_var(src, b);
		else
			cbuf_put(b, c);
	}
}

void expand(const char* c, pbuf_t* wds) {
	cbuf_t buf;
	if (*c == '\0') return;
	cbuf_init(&buf, 256);
	bool b1w = false;

	while (true) {
		switch(getctype(*c)) {
			case CT_CNJ: case CT_SPA:
				cbuf_put(&buf, '\0');
				pbuf_put(wds, cbuf_shrink(&buf));
				if (*c == '\0') return;
				cbuf_init(&buf, 256);
				b1w = false;
				break;
			case CT_RED: case CT_NRM: case CT_COM:
				cbuf_put(&buf, *c);
				b1w = true;
				break;
			case CT_ESC:
				switch(*c) {
					case '\\':
						expand_escpd_char(0, &c, &buf);
						break;
					case '\'': case '"':
						expand_escpd_str(&c, &buf);
						break;
				}
				b1w = true;
				break;
			case CT_VAR:
				if (*c == '$') {
					if (!(b1w = expand_var_split(&c, wds, &buf, b1w))) {
						if (c[1] == '\0') return;
						cbuf_init(&buf, 256);
					}
				} else { // ~
					char* val = get_var("HOME");
					if(val)
						cbuf_putn(&buf, val, strlen(val));
					b1w = true;
				}
				break;
		}
		++c;
	}
}

ecmd_2_t* exec_cmd_2(const cmd_2_t* c2, ecmd_st* st) {
	size_t cmdc = c2->cmdc;
	ecmd_2_t* ecmd = malloc(offsetof(ecmd_2_t, procs) 
								+ cmdc * sizeof(ecmdp_t));
	ecmd->nb_alive = 0;
	ecmd->st       = st;

    int fd_in = -1;
    for (size_t ic = 0; ic < cmdc; ++ic) {
		ecmd->procs[ic].pid = PID_NONE;
		ecmd->procs[ic].st  = 66;
        cmd_1_t          *c = c2->cmds + ic;

        int next_fd_in = -1,
			my_fd_out  = -1;

		if (ic != cmdc - 1) {
            int fds[2] = { -1, -1 };
            int rp     = pipe(fds);
            if (rp < 0) {
                perror("sh pipe");
                exit(1);
            }

            next_fd_in = fds[0];
            my_fd_out  = fds[1];
		}

		pbuf_t argsb;
		char** args = NULL;
		size_t argc = 0;
		builtin_t* blti = NULL;
		var_t* loc_vars = NULL;

		if (c->c.ty == C_BAS) {
			pbuf_init(&argsb, 8);
			expand(c->c.bas, &argsb);
			pbuf_put(&argsb, NULL);
			pbuf_shrink(&argsb);
			args = argsb.c;
			argc = argsb.sz - 1;

			var_t** lv_tail = &loc_vars;
			while (argc > 0) {
				if (argc == 1) {
					if (arg_var_assign(args[0])) {
						ecmd->procs[ic].st = 0;
						goto continue_pipe;
					} else break;
				} else {
					char* setp = strchr(args[0], '=');
					if (setp) {
						var_t* lv = malloc(sizeof(var_t));
						*setp     = '\0';
						lv->name  = args[0];
						char*  v0 = setp + 1;
						lv->val   = malloc(strlen(v0) + 1);
						strcpy(lv->val, v0);
						lv->next  = NULL;
						*lv_tail  = lv;
						lv_tail   = &lv->next;
					} else break;
				}
				--argc;
				++args;
			}

			if (!argc) {
				ecmd->procs[ic].st = 0;
				goto continue_pipe;
			}

			blti = find_builtin(args[0]);
			if (blti) {
				switch(blti->ty) {
					case BLTI_TOP:
						blti = NULL;
						break;
					case BLTI_ASYNC: break;
					case BLTI_SYNC: {
						int mfd_in = ~fd_in ? fd_in : STDIN_FILENO;
						if (0 <= mfd_in && mfd_in < 3)
							mfd_in = st->fds[mfd_in];
						ecmd->procs[ic].st = blti->sfun(argc, args, mfd_in);
						goto continue_pipe;
					}
				}
			}
		}

        int rf = fork();
        if (rf < 0) perror("sh fork\n");

        else if (rf == 0) {// enfant
			// redirections hérités
			apply_reds3(st);

			// on ferme les pipes inutiles
            if (~fd_in && fd_in != STDIN_FILENO) {
				dup2(fd_in, STDIN_FILENO);
				close(fd_in);
			}
            if (~my_fd_out && my_fd_out != STDOUT_FILENO) {
				dup2(my_fd_out, STDOUT_FILENO);
				close(my_fd_out);
			}
            if (~next_fd_in)  close(next_fd_in);

            // redirections
			for (size_t ir = 0; ir < c->r.redc; ++ir) {
				int rds = exec_redir(c->r.reds + ir);
				if (rds) exit(rds);
			}
			
			// exports locaux
			for (var_t* v = loc_vars; v;) {
				setenv(v->name, v->val, 1);
				var_t* vn = v->next;
				free(v);
				v = vn;
			}

	
			if (blti)
				exit(blti->fun(argc, args));

			switch (c->c.ty) {
			case C_BAS:
				execvp(args[0], (const char**)args);
				perror(args[0]);
				exit(1);
			case C_SUB:
				if (!c->c.sub) exit(0);
				start_sub(c->c.sub, true);
				break;
			}
			exit(2);

        } else { // parent
			ecmd->procs[ic].pid = rf;
			++ecmd->nb_alive;
        }

continue_pipe:

		if (args) {
			for (char** it = args; *it; ++it)
				free(*it);
			free(args);
		}

        if (~fd_in)  close(fd_in);
        if (~my_fd_out) close(my_fd_out);
        fd_in = next_fd_in;
    }

	return ecmd;
}

ecmd_st* mk_ecmd_st() {
	ecmd_st* rt = malloc(sizeof(ecmd_st));
	for (int i = 0; i < 3; ++i)
		rt->fds[i] = i;
	rt->stack = NULL;
	rt->reds  = NULL;
	return rt;
}

void start_sub(const cmd_3_t* c3, bool keep_stdin) {
	if (!keep_stdin) {
		int nfd = open("/proc/null", O_RDONLY, 0640);
		if (nfd < 0) {
			perror("sh /proc/null");
			exit(1);
		}
		if (!~dup2(nfd, STDIN_FILENO)) exit(1);
		close(nfd);
	}

	int st;
	ecmd_2_t* ecmd = exec_cmd_3_down(c3, mk_ecmd_st(), &st);
	if (ecmd) exit(run_sub(ecmd));
	else      exit(st);
}

int exec_cmd_3_bg(const cmd_3_t* c3) {
	int rf0 = fork();
	if (rf0 < 0) {
		perror("fork");
		return 1;
	} else if (rf0) { // parent
		int rs;
		pid_t wpid;
		while (true) {
			while (!~(wpid = wait(&rs)));
			if (wpid == rf0) return rs;
			on_child_end(wpid, rs);
		}
	}
	// enfant 0: permet le rattachement à INIT
	int rf1 = fork();
	if (rf1 < 0) {
		perror("fork");
		exit(1);
	} else if (rf1) // enfant 0
		exit(0);

	// enfant 1: sous-shell
	start_sub(c3, false);
}

ecmd_2_t* exec_cmd_3_up(const cmd_3_t* c3, ecmd_st* st,
							int* r_st, int c_st) {
	uint8_t sibnum = c3->sibnum;
	while (c3->parent) {
		c3 = c3->parent;
		switch (c3->ty) {
			case C_CM2:
				fprintf(stderr, "Erreur de structure cmd3\n");
				break;
			case C_BG: return NULL;
			case C_SEQ:
				if (sibnum) break;
				return exec_cmd_3_down(c3->childs[1], st, r_st);
			case C_AND:
				if (sibnum || c_st) break;
				return exec_cmd_3_down(c3->childs[1], st, r_st);
			case C_OR:
				if (sibnum || !c_st) break;
				return exec_cmd_3_down(c3->childs[1], st, r_st);
			case C_RED:
				unreg_reds3(st);
				break;
			case C_WHL:
				if (sibnum == 0) {
					if (c_st) break;
					else if (c3->whl.bdy)
						return exec_cmd_3_down(c3->whl.bdy, st, r_st);
				}
				return exec_cmd_3_down(c3, st, r_st);
		}
		sibnum = c3->sibnum;
	}

	free(st);
	cmd_3_t* d3 = (cmd_3_t*)c3;
	destr_cmd_3(d3);
	free(d3);
	return NULL;
}

ecmd_2_t* exec_cmd_3_down(const cmd_3_t* c3, ecmd_st* st,
							int* r_st) {
	int rts;
	while (true) {
		switch (c3->ty) {
			case C_CM2: {
				ecmd_2_t* ecmd = exec_cmd_2(&c3->cm2, st);
				ecmd->c        = c3;
				return ecmd;
			}
			case C_BG:
				return exec_cmd_3_up(c3, st, r_st,
							exec_cmd_3_bg(c3->childs[0]));
			case C_SEQ: case C_AND: case C_OR:
				c3 = c3->childs[0];
				break;
			case C_RED:
				rts = reg_reds3(st, &c3->red.r);
				if (rts)
					return exec_cmd_3_up(c3, st, r_st, rts);
				c3 = c3->red.c;
				break;
			case C_WHL:
				if (c3->whl.cnd)      c3 = c3->whl.cnd;
				else if (c3->whl.bdy) c3 = c3->whl.bdy;
				else {
					fprintf(stderr, "null loop\n");
					return exec_cmd_3_up(c3, st, r_st, 2);
				}
				break;
		}
	}
}

ecmd_2_t* continue_job(ecmd_2_t* e, int* st) {
	*st = 0;
	const cmd_3_t* c3 = e->c;
	size_t       cmdc = c3->cm2.cmdc;
	for (size_t ic = 0; ic < cmdc; ++ic)
		if (e->procs[ic].st != 0) {
			*st = e->procs[ic].st;
			break;
		}
	return exec_cmd_3_up(c3, e->st, st, *st);
}

bool run_fg(int* st) {
    ecmd_2_t* ecmd = ecmd_llist;
	//printf("run fg %d\n", ecmd->num);
	while (true) {
		int_tstp = false;
		sighandler_t
			prev_hnd_int  = signal(SIGINT,  & int_handler),
			prev_hnd_tstp = signal(SIGTSTP, &tstp_handler);

		int rs;
		pid_t wpid;
		while (ecmd->nb_alive > 0) {
			while (!~(wpid = wait(&rs))) {
				if (int_tstp) {
					signal(SIGINT,  prev_hnd_int);
					signal(SIGTSTP, prev_hnd_tstp);
					printf("\n[%d]\tstopped\n", ecmd->num);
					return false;
				}
			}
			on_child_end(wpid, rs);
		}

		signal(SIGINT,  prev_hnd_int);
		signal(SIGTSTP, prev_hnd_tstp);

		ecmd_2_t* ccm = continue_job(ecmd, st);

		if (!ccm) {
			ecmd_llist = ecmd->next;
			free(ecmd);
			return true;
		}

		ccm->next  = ecmd->next;
		ecmd_llist = ccm;
		free(ecmd);
		ecmd = ccm;
	}
}

int run_sub(ecmd_2_t* ecmd) {
	while (true) {
		ecmd->next = NULL;
		ecmd_llist = ecmd;
		int_tstp = false;
		sighandler_t
			prev_hnd_int  = signal(SIGINT,  &bg_int_handler);

		int rs;
		pid_t wpid;
		while (ecmd->nb_alive > 0) {
			while (!~(wpid = wait(&rs)));
			on_child_end(wpid, rs);
		}

		signal(SIGINT,  prev_hnd_int);

		int st;
		ecmd_2_t* ccm = continue_job(ecmd, &st);

		if (!ccm) {
			ecmd_llist = NULL;
			free(ecmd);
			return st;
		}

		ccm->next  = NULL;
		ecmd_llist = ccm;
		free(ecmd);
		ecmd = ccm;
	}
}
