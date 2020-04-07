#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <assert.h>
#include <signal.h>

pid_t fg_proc_pid = PID_NONE;

void int_handler(int signum) {
    if (signum != SIGINT) {
        printf("bas signal: %d\n", signum);
        return;
    }
    if (kill(fg_proc_pid, SIGINT))
        printf("error sending SIGINT to %d\n", fg_proc_pid);
}


#define NARGS  10
#define NCMD   10

const char *blank = " \t\n";
char *ptr, line[258];
const char *env[1] = { 0 };

struct cmd {
    const char *args[NARGS];
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
    case '>': fill_redir(&curr->outfile); break;

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
    int fd = open(file, flag);
    if (fd < 0) {
        perror("ls");
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

void exec_cmd() {
    for (int i = 0; i < ncmd; i++) {
        struct cmd *c = cmds + i;

        int argc = 0;
        while (c->args[argc]) argc++;

        if (!strcmp(c->args[0], "exit")) binutil_exit(argc, c->args);

        int rc = fork();
        if (rc < 0) {
            printf("error: fork\n");
        } else if (rc == 0) {
            if (c->pipe) {
                int fds[2] = { 42, 42 };
                rc = pipe(fds);
                if (rc < 0) {
                    perror("sh");
                    exit(1);
                }

                rc = fork();
                if (rc < 0) {
                    perror("sh");
                    exit(1);
                }

                if (rc == 0) {
                    // child read on pipe
                    dup2(fds[0], 0);
                    c = c->pipe;
                } else {
                    dup2(fds[1], 1);
                }

                close(fds[0]);
                close(fds[1]);
            }

            // redirections
            mk_redirection(c->infile, O_RDONLY, 0);
            mk_redirection(c->outfile, O_WRONLY, 1);

            rc = execve(c->args[0], c->args, env);
            perror("sh");
            exit(1);
        }

        fg_proc_pid = rc;
        sighandler_t prev_hnd = signal(SIGINT, &int_handler);
        int rs;
        while (!~wait(&rs));
        signal(SIGINT, prev_hnd);
        fg_proc_pid = PID_NONE;

        printf("process %d exited with status %x\n", rc, rs);
    }
}


int main() {
    printf("ecos-shell version 0.1\n");

    int rc;

    while(1) {
        memset(line, 0, 258);
        rc = read(0, line, 256);//TODO: attendre \n + ne pas aller plus loin
        if (rc < 0) {
            printf("an error occurred\n");
            exit(1);
        }

        line[rc] = '\n';

        setup_parse();
        parse_cmd();
        exec_cmd();
    }
}
