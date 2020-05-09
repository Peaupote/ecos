#include "sh.h"

#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#include <util/misc.h>

int blti_exit(int argc, char** argv, int fd_in __attribute__((unused))) {
	int st = argc > 1 ? atoi(argv[1]) : 0;
	sh_exit(st);
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
	int rc = 1;
	while (!int_sint) {

		while (!int_tstp && !int_sint && (rc = read(fd_in, seg, 256)) > 0) {
			buf.sz -= 256 - rc;
			for (int i = 0; i < rc; ++i) {
				if (seg[i] == '\n') {
					lseek(fd_in, -rc + i+1, SEEK_CUR);
					seg[i] = '\0';

					char* sv;
					char* word = strtok_rnul(buf.c, " ", &sv);
					for(int argi = 1; argi < argc; ++argi) {
						if (word)
							var_set(args[argi], strdup(word));
						else {
							cbuf_destr(&buf);
							return 1;
						}
						word = strtok_rnul(NULL, " ", &sv);
					}
					cbuf_destr(&buf);
					return 0;
				}
			}
			seg = cbuf_mkn(&buf, 256);
		}

		if (is_subsh && rc < 0 && errno == EINTR) 
			while (int_tstp && !int_sint) pause();
		else break;
	}
	cbuf_destr(&buf);
	return 1;
}

int blti_kill(int argc, char** args, int fd_in __attribute__((unused))) {
	int argi   = 1;
	int signum = SIGINT;
	for (; argi < argc; ++argi) {
		if (*args[argi] == '-')
			signum = atoi(args[argi] + 1);
		else break;
	}
	int rts = 0;
	for (; argi < argc; ++argi) {
		pid_t pid = atoi(args[argi]);
		int rts1  = kill(pid, signum);
		if (rts1) {
			if (!rts) rts = rts1;
			char buf[20];
			sprintf(buf, "%d", pid);
			perror(buf);
		}
	}
	return rts;
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

	printf("[%d]\t", e->st->num);
	pp_cmd_3(stdout, 'c', cmd_top(e->c));
	printf("\n");

    return continue_fg(st);
}

int blti_jobs(int argc __attribute__((unused)),
		char** args __attribute__((unused))) {
	
	for (ecmd_2_t* e = ecmd_llist; e; e = e->next) {
		printf("[%d]\t", e->st->num);
		pp_cmd_3(stdout, 'c', cmd_top(e->c));
		printf("\n");
	}

	return 0;
}

int blti_echo(int argc, char *argv[]) {
	int argi = 1;
	bool itp = false;
	bool nwl = true;

	for (; argi < argc; ++argi) {
		const char* arg = argv[argi];
		if (arg[0] == '-' && arg[1]) {
			for (const char* c = arg + 1; *c; ++c)
				switch (*c) {
					case 'e': case 'E': case 'n': continue;
					default: goto start_echo;
				}
			for (const char* c = arg + 1; *c; ++c)
				switch (*c) {
					case 'e': itp = true;  continue;
					case 'E': itp = false; continue;
					case 'n': nwl = false; continue;
				}
		} else break;
	}
start_echo:
	for (int mi0 = argi; argi < argc; ++argi) {
		if (argi > mi0) printf(" ");
		if (itp) {
			for (const char *c = argv[argi]; *c; ++c) {
				if (*c == '\\') {
					switch (*++c) {
						case 'n':  fputc('\n', stdout);    break;
						case 't':  fputc('\t', stdout);    break;
						case 'e':  fputc('\033', stdout);  break;
						case '\\': fputc('\\', stdout);    break;
						default: --c; fputc('\\', stdout); break;
					}
				} else fputc(*c, stdout);
			}
		} else
			printf("%s", argv[argi]);
	}

	if (nwl) printf("\n");
	else fflush(stdout);
    return 0;
}

#define GEN_SBLTI(N, T, F, Y, E) \
static char sblti_n_##N[]=#N;\
static builtin_t sblti_##N={.name=sblti_n_##N,.ty= T,.exp=E,.F=(Y)&blti_##N};
#define GEN_SBLTI_ASYNC(N, E) GEN_SBLTI(N,BLTI_ASYNC,fun,builtin_f, E)
#define GEN_SBLTI_SYNC(N, E)  GEN_SBLTI(N,BLTI_SYNC,sfun,builtin_syf, E)
#define GEN_SBLTI_TOP(N) GEN_SBLTI(N,BLTI_TOP,tfun,builtin_top, false)
GEN_SBLTI_SYNC(exit, true)
GEN_SBLTI_SYNC(cd,   false)
GEN_SBLTI_SYNC(export, false)
GEN_SBLTI_SYNC(read, false)
GEN_SBLTI_SYNC(kill, true)
GEN_SBLTI_TOP(fg)
GEN_SBLTI_ASYNC(jobs, false)
GEN_SBLTI_ASYNC(echo, true)
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
	add_builtin(&sblti_echo);
	add_builtin(&sblti_kill);
}

builtin_t* find_builtin(const char* name) {
	unsigned char h = hash_str8(name, 0);
	for (builtin_t* it = builtins[h]; it; it = it->next)
		if (!strcmp(name, it->name))
			return it;
	return NULL;
}

