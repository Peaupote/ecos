#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>

#include <stdbool.h>

struct exd_cmd {
	int    num;
	struct exd_cmd* next;
	int    nb_proc;
	int    nb_alive;
	pid_t  procs[];
};

struct exd_cmd* exd_llist = NULL;
int    next_exd_num = 0;

struct exd_cmd* fg_exd = NULL;
bool   int_tstp;

struct exd_proc_ref {
	struct exd_cmd** cmd;
	int    num;
};
struct exd_proc_ref find_exd_proc(pid_t pid) {
	for (struct exd_cmd** cmdr = &exd_llist; *cmdr; cmdr = &(*cmdr)->next) {
		struct exd_cmd* cmd = *cmdr;
		for (int i = 0; i < cmd->nb_proc; ++i) {
			if (cmd->procs[i] == pid) {
				struct exd_proc_ref rt = {cmdr, i};
				return rt;
			}
		}
	}
	struct exd_proc_ref rt = {NULL, 0};
	return rt;
}
struct exd_cmd** find_exd_num(int num) {
	for (struct exd_cmd** cmdr = &exd_llist; *cmdr; cmdr = &(*cmdr)->next)
		if ((*cmdr)->num == num)
			return cmdr;
	return NULL;
}
struct exd_cmd** on_child_end(pid_t pid) {
	struct exd_proc_ref rf = find_exd_proc(pid);
	if (!rf.cmd) {
		printf("couldn't find %d\n", (int)pid);
		return NULL;
	}
	struct exd_cmd* cmd = *rf.cmd;
	cmd->procs[rf.num]  = PID_NONE;
	--cmd->nb_alive;
	return rf.cmd;
}

void int_handler(int signum) {
    if (signum != SIGINT) {
        printf("bad signal: %d\n", signum);
        return;
    }
	for (int i = 0; i < fg_exd->nb_proc; ++i)
		if (~fg_exd->procs[i] && kill(fg_exd->procs[i], SIGINT))
    		perror("error sending SIGINT");
}

void tstp_handler(int signum __attribute__((unused))) {
	for (int i = 0; i < fg_exd->nb_proc; ++i)
		if (~fg_exd->procs[i] && kill(fg_exd->procs[i], SIGTSTP))
    		perror("error sending SIGSTP");
	int_tstp = true;
}



#define NARGS  10
#define NCMD   10

const char *blank = " \t\n";
char *ptr, line[258];

struct cmd {
    const char *args[NARGS];
    int oflags;
    char *infile, *outfile;
    struct cmd *pipe;
} cmds[NCMD];

int ncmd;
struct cmd *curr;
char **aptr;

void setup_parse() {
    ptr     = line;
    curr    = cmds;
    ncmd    = 0;
    memset(cmds, 0, NCMD * sizeof(struct cmd));
}

void setup_curr_cmd() {
    aptr         = (char**)curr->args;
    curr->infile  = 0;
    curr->outfile = 0;
    memset(curr->args, 0, 10);
}

static inline void skip(const char *str) {
    for(++ptr; *ptr && index(str, *ptr); ++ptr);
}

static inline void fill_redir(char **redir) {
    *aptr++ = 0;
    skip(blank);
    *redir = ptr;
}

int parse_simple_cmd() {
    char c;
start:
    switch (*ptr) {
    case 0:
    case '|':
    case '\n':
        goto end;

    case '"':
    case '\'': // escape
        for (c = *ptr, *aptr++ = ++ptr; *ptr != c; ++ptr);
        *ptr++ = 0;
        goto start;

    case '\t':
    case '\r':
    case ' ' :
        skip(blank);
        goto start;

    case '<' : fill_redir(&curr->infile); break;
    case '>':
        curr->oflags = O_CREAT;
        if (ptr[1] == '>') ++ptr, curr->oflags |= O_APPEND;
        else curr->oflags |= O_TRUNC;
        fill_redir(&curr->outfile);
        break;

    default:
        *aptr++ = ptr; // set argv
        break;
    }

    for (++ptr; !index(blank, *ptr); ++ptr);
    *ptr++ = 0;
    goto start;

end:
    *aptr++ = 0;
    return 0;
}

void parse_cmd() {
    struct cmd *save;
    curr = cmds + ncmd++;
    setup_curr_cmd();
    parse_simple_cmd();
    switch (*ptr) {
    case 0:
    case '\n':
        *ptr = 0;
        break;

    case '|':
        ++ptr;
        save = curr;
        parse_cmd();
        save->pipe = curr;
    }
}

void mk_redirection(char *file, int flag, int redir) {
    if (!file) return;
    int fd = open(file, flag, 0640); // TODO : umask
    if (fd < 0) {
        char buf[256];
        sprintf(buf, "sh: %s", file);
        perror(buf);
        exit(1);
    }

    // equivalent to dup2
    close(redir);
    dup(fd);
    close(fd);
}

void binutil_exit(int argc, const char *argv[]) {
    int code = 0;
    if (argc > 1) code = atoi(argv[1]);
    exit(code);
}

void run_fg() {
	struct exd_cmd* ecmd = fg_exd = exd_llist;
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
				fg_exd = NULL;
				printf("\tbg %d\n", ecmd->num);
				return;
			}
		}
		struct exd_cmd** er = on_child_end(wpid);
		if (!er) continue;
		struct exd_cmd*   e = *er;
		if (e != ecmd && e->nb_alive <= 0) {
			*er = e->next;
			free(e);
		}
	}
		
	signal(SIGINT,  prev_hnd_int);
	signal(SIGTSTP, prev_hnd_tstp);
	fg_exd = NULL;
	exd_llist = ecmd->next;
	free(ecmd);
}

void fg(int argc, const char *argv[]) {
	int num;
	if (argc != 2 || sscanf(argv[1], "%d", &num) != 1) return;
	struct exd_cmd** er = find_exd_num(num);
	if (!er) {
		printf("Aucune commande corespondante: %d\n", num);
		return;
	}
	
	struct exd_cmd* e = *er;
	*er       = e->next;
	e->next   = exd_llist;
	exd_llist = e;
	
	for (int i = 0; i < e->nb_proc; ++i)
		if (~e->procs[i] && kill(e->procs[i], SIGCONT))
    		perror("error sending SIGCONT");

	run_fg();
}

void exec_cmd() {
    printf("\033d");

	if (ncmd == 1 && !strcmp(cmds->args[0], "fg")) {
        int argc = 0;
        while (cmds->args[argc]) argc++;
		fg(argc, cmds->args);
		return;
	}

	struct exd_cmd* ecmd = malloc(sizeof(struct exd_cmd) + ncmd * sizeof(pid_t));
	ecmd->num  = next_exd_num++;
	ecmd->next = exd_llist;
	exd_llist  = ecmd;
	ecmd->nb_proc  = ncmd;
	ecmd->nb_alive = 0;

    int fd_in = STDIN_FILENO;
    for (int i = 0; i < ncmd; i++) {
        struct cmd  *c = cmds + i;
		ecmd->procs[i] = PID_NONE;

        int argc = 0;
        while (c->args[argc]) argc++;

        if (!strcmp(c->args[0], "exit")) binutil_exit(argc, c->args);

        int next_fd_in = STDIN_FILENO;
        int my_fd_out  = STDOUT_FILENO;
        if (c->pipe) {
            int fds[2] = { 42, 42 };
            int rp = pipe(fds);
            if (rp < 0) {
                perror("sh");
                exit(1);
            }

            next_fd_in = fds[0];
            my_fd_out  = fds[1];
        }

        int rf = fork();
        if (rf < 0) {
            printf("error: fork\n");
        } else if (rf == 0) {

            if (fd_in != STDIN_FILENO)
                dup2(fd_in, STDIN_FILENO);
            if (my_fd_out != STDOUT_FILENO)
                dup2(my_fd_out, STDOUT_FILENO);
            if (next_fd_in)
                close(next_fd_in);

            // redirections
            mk_redirection(c->infile,  c->oflags|O_RDONLY, 0);
            mk_redirection(c->outfile, c->oflags|O_WRONLY, 1);

            execvp(c->args[0], c->args);
            perror("sh");
            exit(1);

        } else {
			ecmd->procs[i] = rf;
			++ecmd->nb_alive;
		}

        if (fd_in != STDIN_FILENO)
            close(fd_in);
        if (my_fd_out != STDOUT_FILENO)
            close(my_fd_out);
        fd_in = next_fd_in;

    }

    if (fd_in != STDIN_FILENO)
        close(fd_in);

	run_fg();

    /* printf("process %d exited with status %x\n", rc, rs); */
}

int main() {
    printf("ecos-shell version 0.1\n");
    int rc;

    while(1) {
        printf("\033psh> \033;");
        memset(line, 0, 258);
        rc = read(0, line, 256);
        line[rc] = '\0';//TODO: attendre \n + ne pas aller plus loin

        if (rc == 0) return 0;
        if (rc == 1) continue;
        if (rc < 0) {
            printf("an error occurred\n");
            exit(1);
        }

        setup_parse();
        parse_cmd();
        exec_cmd();
    }
}
