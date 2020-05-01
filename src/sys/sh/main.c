#include "sh.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <dirent.h>

#include <util/misc.h>

ecmd_2_t*   ecmd_llist   = NULL;
int         next_cmd_num = 0;
char        line[513];
bool        update_cwd = true;
static char cwd[256] = "";

static inline bool stat_eq(struct stat* a, struct stat* b) {
	return a->st_ino == b->st_ino && a->st_dev == b->st_dev;
}

static void do_update_cwd() {
	struct stat st = { 0 }, p = { 0 };
	stat(".", &st);
	stat("..", &p);
	if (stat_eq(&st, &p)) {
		cwd[0] = '/';
		cwd[1] = '\0';
		return;
	}
    DIR *dirp = opendir("..");
	if (!dirp) {
		perror("cwd:dirp");
		return;
	}

    struct dirent *dir;
	while (true) {
		if (!(dir = readdir(dirp))) {
			fprintf(stderr, "cwd\n");
			cwd[0] = '\0';
			closedir(dirp);
			return;
		}
		if (p.st_dev == st.st_dev) {
			if (dir->d_ino == st.st_ino) break;
		} else {
			char* v = dir->d_name - 3;
			v[0] = v[1] = '.';
			v[2] = '/';
			struct stat c;
			stat(v, &c);
			if (stat_eq(&c, &st)) break;
		}
	}

	size_t len = min_size_t(dir->d_name_len, 255);
	memcpy(cwd, dir->d_name, len);
	cwd[len] = '\0';

	closedir(dirp);
	update_cwd = false;
	return;
}

int main() {
    printf("ecos-shell version 0.1\n");
	init_builtins();

	int   buf_rem = 0;
	char* buf = NULL;

    while(1) {
		if (update_cwd) do_update_cwd();
	
		for (int i = 0; i < buf_rem; ++i)//may overlap
			line[i] = buf[i];

		int rem = 512 - buf_rem;
		buf = line;

		printf("\033p%s> \033;", cwd); fflush(stdout);
		while (true) {
			int rc = read(STDIN_FILENO, buf, rem);
			if (rc == 0) return 0;
			if (rc < 0) {
				perror("sh - input");
				return 1;
			}
			for (int i = 0; i < rc; ++i) {
				if (*buf == '\n') {
					*buf = '\0';
					buf_rem = rc - i - 1;
					++buf;
					goto end_input_read;
				}
				++buf;
			}
			rem -= rc;
			printf("\033p\033;"); fflush(stdout);
		}
end_input_read:;	
		
		cmd_3_t* cmd;
		const char* curs = line;
		//printf("begin parse\n");
		int pst = parse_cmd_3c(&curs, &cmd);
		
		//printf("parse_st = %d\n", pst);
		if (pst > 1 || *curs != '\0') {
			fprintf(stderr, "Syntax error (%d) near char %d\n", 
					pst, (int)(curs - line));
			continue;
		} if (pst == 1)
			continue;
		cmd->parent = NULL;

    	printf("\033d"); fflush(stdout);

		int st = 0;
		if (cmd->ty == C_CM2
				&& cmd->cm2.cmdc == 1
				&& cmd->cm2.cmds[0].redc   == 0
				&& cmd->cm2.cmds[0].cmd.ty == C_BAS) {
			
			char *cstr      = cmd->cm2.cmds[0].cmd.bas;
			char *end_cname = strchrnul(cstr, ' ');
			const char end_cnamec = *end_cname;
			*end_cname   = '\0';
			builtin_t* b = find_builtin(cstr);
			*end_cname   = end_cnamec;

			if (b && b->ty == BLTI_TOP) {
				pbuf_t args;
				pbuf_init(&args, 8);
				expand(cstr, &args);
				pbuf_put(&args, NULL);
				pbuf_shrink(&args);

				bool ended = b->tfun(args.sz - 1, args.c, &st);

				for (char** it = args.c; *it; ++it)
					free(*it);
				free(args.c);
				destr_cmd_3(cmd);
				free(cmd);
				if (ended) goto check_status;
				else       continue;
			}
		}
		ecmd_2_t* ecmd = exec_cmd_3_down(cmd, &st);
		if (ecmd) {
			ecmd->num  = next_cmd_num++;
			ecmd->next = ecmd_llist;
			ecmd_llist = ecmd;
			if (!run_fg(&st)) continue;
		}
check_status:
		if (st)
			fprintf(stderr, "exit status %d\n", st);
    }
}
