#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>

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
    int fd = open(file, flag|O_CREAT, 0640); // TODO : umask
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

void exec_cmd() {
    printf("\033d");

    int fd_in = STDIN_FILENO;
    int cpid  = PID_NONE;
    for (int i = 0; i < ncmd; i++) {
        struct cmd *c = cmds + i;

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
            mk_redirection(c->infile,  O_RDONLY, 0);
            mk_redirection(c->outfile, O_WRONLY, 1);

            execvp(c->args[0], c->args);
            perror("sh");
            exit(1);

        } else cpid = rf;

        if (fd_in != STDIN_FILENO)
            close(fd_in);
        if (my_fd_out != STDOUT_FILENO)
            close(my_fd_out);
        fd_in = next_fd_in;

    }

    if (fd_in != STDIN_FILENO)
        close(fd_in);

    fg_proc_pid = cpid;
    sighandler_t prev_hnd;
    if (~cpid)
        prev_hnd = signal(SIGINT, &int_handler);
    int rs;
    for (int i = 0; i < ncmd; i++) //TODO: utiliser errno
        while (!~wait(&rs));
    if (~cpid) {
        signal(SIGINT, prev_hnd);
        fg_proc_pid = PID_NONE;
    }

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
