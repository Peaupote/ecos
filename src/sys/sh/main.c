#include "sh.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

ecmd_2_t* ecmd_llist   = NULL;
int       next_cmd_num = 0;
char      line[258];

/*
void fg(int argc, const char *argv[]) {
    int num;
    if (argc != 2 || sscanf(argv[1], "%d", &num) != 1) return;
    ecmd_2_t** er = find_exd_num(num);
    if (!er) {
        printf("Aucune commande corespondante: %d\n", num);
        return;
    }

    struct exd_cmd* e = *er;
    *er        = e->next;
    e->next    = ecmd_llist;
    ecmd_llist = e;

    for (int i = 0; i < e->nb_proc; ++i)
        if (~e->procs[i].pid && kill(e->procs[i].pid, SIGCONT))
            perror("error sending SIGCONT");

    run_fg();
}
*/


int main() {
    printf("ecos-shell version 0.1\n");
    int rc;

    while(1) {
        printf("\033psh> \033;"); fflush(stdout);
        memset(line, 0, 258);
        rc = read(0, line, 256);
        line[rc] = '\0';//TODO: attendre \n + ne pas aller plus loin
		
        if (rc == 0) return 0;
        if (rc == 1) continue;
        if (rc < 0) {
            printf("an error occurred\n");
            exit(1);
        }
		
		cmd_3_t* cmd;
		const char* curs = line;
		//printf("begin parse\n");
		int pst = parse_cmd_3c(&curs, &cmd);
		
		//printf("parse_st = %d\n", pst);
		if (pst > 1) {
			fprintf(stderr, "Syntax error (%d) near char %d\n", 
					pst, (int)(curs - line));
			continue;
		} if (pst == 1)
			continue;
		cmd->parent = NULL;

		pp_cmd_3(stdout, 'c', cmd);
		printf("\n");
		
    	printf("\033d"); fflush(stdout);
	
		int st = 0;
		ecmd_2_t* ecmd = exec_cmd_3_down(cmd, &st);
		if (ecmd) {
			ecmd->num  = next_cmd_num++;
			ecmd->next = ecmd_llist;
			ecmd_llist = ecmd;
			if (!run_fg(&st)) continue;
		}
		if (st)
			fprintf(stderr, "exit status %d\n", st);
    }
}
