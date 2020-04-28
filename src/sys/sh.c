#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>

#include <stdbool.h>

char *ptr, line[258];

// --Parse--

enum redir_ty {
    IN_DUP,  IN_FILE,
    OUT_DUP, OUT_TRUNC, OUT_APPEND
};
typedef struct {
    int           fd;
    enum redir_ty type;
    union {
        char*     filename;
        int       src_fd;
    };
} redir_t;

typedef struct {
    int     argc;
    char*   args;
} cmd_0a_t;

typedef struct {
    cmd_0a_t cmd;
    size_t   redc;
    redir_t* reds;
} cmd_1_t;

typedef struct {
    size_t   cmdc;
    cmd_1_t* cmds;
} cmd_2_t;


void destr_redir(redir_t* rd) {
    switch (rd->type) {
        case IN_FILE: case OUT_APPEND: case OUT_TRUNC:
            free(rd->filename);
            break;
        case IN_DUP: case OUT_DUP: break;
    }
}

void destr_cmd_0a(cmd_0a_t* ptr) {
    free(ptr->args);
}

void destr_cmd_1(cmd_1_t* ptr) {
    destr_cmd_0a(&ptr->cmd);
    for (size_t rd = 0; rd < ptr->redc; ++rd)
        destr_redir(ptr->reds + rd);
    free(ptr->reds);
    ptr->reds = NULL;
}

void destr_cmd_2(cmd_2_t* ptr) {
    for (size_t c = 0; c < ptr->cmdc; ++c)
        destr_cmd_1(ptr->cmds + c);
    free(ptr->cmds);
    ptr->cmds = NULL;
}


int getctype(char c) {
    switch (c) {
        case  ' ': case '\f': case '\n': case '\r':
        case '\t': case '\v': case '\0':
        case '(': case ')': case ';': case '&': case '|':
            return 0;
        case '>': case '<':
            return -1;
        case '\\':
            return 2;
        default:
            return 1;
    }
}

char getechar(char c) {
    switch (c) {
        case 'n': return '\n';
        case 't': return '\t';
        case '\\': return '\\';
        default:  return c;
    }
}

const char* wait_non_space(const char* c) {
    while (*c && isspace(*c))
        ++c;
    return c;
}

int count_eword(const char** cr) {
    const char *c = *cr;
    char ec = *c;
    int rc  = 0;
    for (++c; *c != ec; ++rc)
        switch (*c) {
            case '\0': *cr = c; return -2; //ERREUR
            case '\\':
                if (c[1] == '\0') {
                    *cr = c + 1;
                    return -2; //ERREUR
                }
                c += 2;   break;
            default: ++c; break;
        }
    *cr = c + 1;
    return rc;
}
int count_word(const char** cr) {
    const char *c0 = wait_non_space(*cr);
    const char *c  = c0;
    int rc = 0;

    if (*c == '"' || *c == '\'') {
        *cr = c;
        return count_eword(cr);
    }

    int ct;
    for (; (ct = getctype(*c)) > 0; c += ct) {
        ++rc;
        if (ct == 2 && c[1] == '\0') {
            *cr = c + 1;
            return -2; //ERREUR
        }
    }

    if (ct == -1) { // >, <
        *cr = c0;
        return -1;
    } else {
        *cr = c;
        return rc > 0 ? rc : -1;
    }
}

char* read_eword(char* dst, const char** cr) {
    const char *c = *cr;
    char ec = *c;

    for (++c; *c != ec;)
        if (*c == '\\') {
            *dst++ = getechar(c[1]);
            c += 2;
        } else {
            *dst++ = *c;
            ++c;
        }

    *cr = c + 1;
    return dst;
}
char* read_word(char* dst, const char** cr) {
    const char *c = wait_non_space(*cr);
    if (*c == '"' || *c == '\'') {
        *cr = c;
        return read_eword(dst, cr);
    }

    for (int ct; (ct = getctype(*c)) > 0; c += ct)
        *dst++ = *c == '\\' ? getechar(c[1]) : *c;

    *cr = c;
    return dst;
}

int parse_cmd_0(const char** cr, cmd_0a_t* rt) {
    //printf("read_cmd_0 @ line+%d\n", *cr - line);
    const char *c0 = *cr;
    int args_sz = 0;
    int argc    = 0;
    int rc;

    for (; (rc = count_word(cr)) >= 0; ++argc)
        args_sz += rc + 1;
    if (rc < -1)  return 2;
    if (!args_sz) return 1;

    char* args = malloc(args_sz * sizeof(char));
    rt->argc   = argc;
    rt->args   = args;
    *cr = c0;
    for (int i = 0; i < argc; ++i) {
        args    = read_word(args, cr);
        *args++ = '\0';
    }

    return 0;
}

bool parse_fd(const char** cr, int* rt) {
    if (isdigit(**cr)) {
        *rt = 0;
        do {
            *rt = 10 * (*rt) + **cr - '0';
        } while(isdigit(*++*cr));
        return true;
    }
    return false;
}

int parse_redt(const char** cr) {
    const char* c = *cr;
    int rt;
    if (*c == '<')
        rt = IN_FILE;
    else if (*c == '>' && c[1] == '>') {
        *cr = c + 2;
        return OUT_APPEND;
    } else if (*c == '>')
        rt = OUT_TRUNC;
    else return -1;

    c = wait_non_space(c + 1);
    if (*c == '&') {
        *cr = c + 1;
        return rt - 1;//dup
    } else {
        *cr = c;
        return rt;
    }
}

int parse_redir(const char** cr, redir_t* rt) {
    //printf("parse_redir @ line+%d\n", *cr - line);
    *cr = wait_non_space(*cr);

    if (**cr != '<' && **cr != '>') {
        if (!parse_fd(cr, &rt->fd))
            return 1;
        int ty = parse_redt(cr);
        if (!~ty) return 2;
        rt->type = (enum redir_ty) ty;
    } else {
        int ty = parse_redt(cr);
        if (!~ty) return 1;
        rt->type = (enum redir_ty) ty;
        rt->fd = ty <= IN_FILE ? 0 : 1;
    }

    switch (rt->type) {
        case IN_DUP: case OUT_DUP:
            *cr = wait_non_space(*cr);
            return parse_fd(cr, &rt->src_fd) ? 0 : 2;

        case IN_FILE: case OUT_TRUNC: case OUT_APPEND: {
            const char* c = *cr;
            int cc = count_word(&c);
            if (cc < 0) return 2;
            rt->filename = malloc((cc + 1) * sizeof(char));
            *read_word(rt->filename, cr) = '\0';
            return 0;
        }
    }
    return 2;//never reached
}

int parse_cmd_1(const char** cr, cmd_1_t* rt) {
    //printf("parse_cmd_1 @ line+%d\n", *cr - line);
    int rts = parse_cmd_0(cr, &rt->cmd);
    if (rts) return rts;

    size_t rd_sp = 1;
    rt->reds     = malloc(rd_sp * sizeof(redir_t));
    rt->redc = 0;
    while (true) {
        if (rt->redc == rd_sp) {
            rd_sp   *= 2;
            rt->reds = realloc(rt->reds, rd_sp * sizeof(redir_t));
        }
        rts = parse_redir(cr, rt->reds + rt->redc);
        if (!rts) ++rt->redc;
        else if (rts == 1) {
            rt->reds = realloc(rt->reds, rt->redc * sizeof(redir_t));
            return 0;
        } else {
            destr_cmd_1(rt);
            return rts;
        }
    }
}

int parse_cmd_2(const char** cr, cmd_2_t* rt) {
    //printf("parse_cmd_2 @ line+%d\n", *cr - line);
    size_t c_sp = 1;
    rt->cmds    = malloc(c_sp * sizeof(cmd_1_t));
    rt->cmdc    = 0;
    while (true) {
        if (rt->cmdc == c_sp) {
            c_sp    *= 2;
            rt->cmds = realloc(rt->cmds, c_sp * sizeof(cmd_1_t));
        }
        int rts = parse_cmd_1(cr, rt->cmds + rt->cmdc);
        if (rts) {
            destr_cmd_2(rt);
            return rts == 1 ? 2 : rts;
        }
        ++rt->cmdc;
        *cr = wait_non_space(*cr);
        if (**cr != '|' || (*cr)[1] == '|') {
            rt->cmds = realloc(rt->cmds, rt->cmdc * sizeof(cmd_1_t));
            return 0;
        }
        ++*cr;
    }
}

// --Pretty print--
void pp_word(FILE* f, const char* c) {
    for (; *c; ++c) {
        switch (*c) {
            case '\n': fwrite("\\n",  1, 2, f); break;
            case '\t': fwrite("\\t",  1, 2, f); break;
            case '\\': fwrite("\\\\", 1, 2, f); break;
            case ' ':  fwrite(" ",    1, 1, f); break;
            default:   fwrite(c,      1, 1, f); break;
        }
    }
}
void pp_cmd_0(FILE* f, const cmd_0a_t* c) {
    const char* a = c->args;
    for (int i = 0; i < c->argc; ++i) {
        pp_word(f, a);
        fwrite(" ", 1, 1, f);
        a += strlen(a) + 1;
    }
}
void pp_redir(FILE* f, const redir_t* r) {
    fprintf(f, "%d", r->fd);
    switch (r->type) {
        case IN_DUP:     fprintf(f, "<&%d ",   r->src_fd); break;
        case OUT_DUP:    fprintf(f, ">&%d ",   r->src_fd); break;
        case IN_FILE:    fprintf(f, "<%s ",  r->filename); break;
        case OUT_TRUNC:  fprintf(f, ">%s ",  r->filename); break;
        case OUT_APPEND: fprintf(f, ">>%s ", r->filename); break;
    }
}
void pp_cmd_1(FILE* f, const cmd_1_t* c) {
    pp_cmd_0(f, &c->cmd);
    for (size_t r = 0; r < c->redc; ++r)
        pp_redir(f, c->reds + r);
}
void pp_cmd_2(FILE* f, const cmd_2_t* c) {
    for (size_t i = 0; i < c->cmdc; ++i) {
        if (i) fwrite(" |  ", 1, 4, f);
        pp_cmd_1(f, c->cmds + i);
    }
}

// --Exécution--

typedef struct {
    pid_t pid;
    int st;
} ecmdp_t;
typedef struct ecmd_2_t ecmd_2_t;
struct ecmd_2_t {
    int              num;
    const cmd_2_t   *cmd;
    struct ecmd_2_t *next;
    int              nb_alive;
    ecmdp_t          procs[];
};

ecmd_2_t* ecmd_llist   = NULL;
int       next_cmd_num = 0;

bool      int_tstp;


struct ecmdp_ref {
    ecmd_2_t** cmd;
    int        num;
};
struct ecmdp_ref find_exd_proc(pid_t pid) {
    for (ecmd_2_t** cmdr = &ecmd_llist; *cmdr; cmdr = &(*cmdr)->next) {
        ecmd_2_t* cmd = *cmdr;
        for (size_t i = 0; i < cmd->cmd->cmdc; ++i) {
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

void int_handler(int signum) {
    if (signum != SIGINT) {
        printf("bad signal: %d\n", signum);
        return;
    }
    ecmd_2_t* fgc = ecmd_llist;
    for (size_t i = 0; i < fgc->cmd->cmdc; ++i)
        if (~fgc->procs[i].pid && kill(fgc->procs[i].pid, SIGINT))
            perror("error sending SIGINT");
}
void tstp_handler(int signum __attribute__((unused))) {
    ecmd_2_t* fgc = ecmd_llist;
    for (size_t i = 0; i < fgc->cmd->cmdc; ++i)
        if (~fgc->procs[i].pid && kill(fgc->procs[i].pid, SIGTSTP))
            perror("error sending SIGSTP");
    int_tstp = true;
}

bool run_fg() {
    ecmd_2_t* ecmd = ecmd_llist;
    //printf("run fg %d\n", ecmd->num);
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
                printf("\tbg %d\n", ecmd->num);
                return false;
            }
        }
        ecmd_2_t** er = on_child_end(wpid, rs);
        if (!er) continue;
        ecmd_2_t*   e = *er;
        if (e != ecmd && e->nb_alive <= 0) {
            *er = e->next;
            free(e);//TODO
        }
    }

    signal(SIGINT,  prev_hnd_int);
    signal(SIGTSTP, prev_hnd_tstp);
    ecmd_llist = ecmd->next;
    free(ecmd);//TODO
    return true;
}



int exec_redir_file(int fd, const char* file, int flag) {
    int ffd = open(file, flag, 0640); // TODO: umask
    if (fd < 0) {
        char buf[256];
        sprintf(buf, "sh rdf: %s", file);// TODO size check
        perror(buf);
        return 1;
    }
    if (!~dup2(ffd, fd)) return 1;
    close(ffd);
    return 0;
}
int exec_redir_dup(int fd, int srcfd) {
    return ~dup2(srcfd, fd) ? 0 : 1;
}
int exec_redir(const redir_t* r) {
    switch (r->type) {
        case IN_DUP:
            return exec_redir_dup(r->fd, r->src_fd);
        case IN_FILE:
            return exec_redir_file(r->fd, r->filename, O_RDONLY);
        case OUT_DUP:
            return exec_redir_dup(r->fd, r->src_fd);
        case OUT_TRUNC:
            return exec_redir_file(r->fd, r->filename,
                    O_WRONLY | O_CREAT | O_TRUNC);
        case OUT_APPEND:
            return exec_redir_file(r->fd, r->filename,
                    O_WRONLY | O_CREAT | O_APPEND);
    }
    return 2;//never reached
}

void exec_builtin_cd(int argc, char *argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: cd dir");
        return;
    }

    // skip cd
    while (*argv) ++argv;
    ++argv;

    // find dirname
    while (*argv == ' ') ++argv;
    chdir(argv);
}

ecmd_2_t* exec_cmd_2(const cmd_2_t* c2) {

    // quick fix to test builtin cd
    if (c2->cmds->cmd.argc > 0 && !strcmp(c2->cmds->cmd.args, "cd")) {
        exec_builtin_cd(c2->cmds->cmd.argc, c2->cmds->cmd.args);
    }

    ecmd_2_t* ecmd = malloc(offsetof(ecmd_2_t, procs)
                        + c2->cmdc * sizeof(ecmdp_t));
    ecmd->cmd      = c2;
    ecmd->nb_alive = 0;

    int fd_in = STDIN_FILENO;

    for (size_t i = 0; i < c2->cmdc; ++i) {
        ecmd->procs[i].pid = PID_NONE;
        cmd_1_t         *c = c2->cmds + i;

        int next_fd_in = STDIN_FILENO;
        int my_fd_out  = STDOUT_FILENO;

        if (i != c2->cmdc - 1) {
            int fds[2] = { -1, -1 };
            int rp     = pipe(fds);
            if (rp < 0) {
                perror("sh pipe");
                exit(1);
            }

            next_fd_in = fds[0];
            my_fd_out  = fds[1];
        }

        int rf = fork();
        if (rf < 0) perror("sh fork\n");

        else if (rf == 0) {// enfant
            // on ferme les pipes inutiles
            if (fd_in      != STDIN_FILENO) dup2(fd_in, STDIN_FILENO);
            if (my_fd_out != STDOUT_FILENO) dup2(my_fd_out, STDOUT_FILENO);
            if (next_fd_in != STDIN_FILENO) close(next_fd_in);

            // redirections
            for (size_t ir = 0; ir < c->redc; ++ir) {
                int rds = exec_redir(c->reds + i);
                if (rds) exit(rds);
            }

            const char** args = malloc(sizeof(const char*) * (c->cmd.argc+1));
            const char*    it = c->cmd.args;
            for (int ia = 0; ia < c->cmd.argc; ++ia) {
                args[ia] = it;
                while(*it) ++it;
                ++it;
            }
            args[c->cmd.argc] = NULL;

            execvp(args[0], args);
            perror("sh exv");
            exit(1);

        } else { // parent
            ecmd->procs[i].pid = rf;
            ++ecmd->nb_alive;
        }

        if (fd_in     != STDIN_FILENO)  close(fd_in);
        if (my_fd_out != STDOUT_FILENO) close(my_fd_out);
        fd_in = next_fd_in;

    }

    if (fd_in != STDIN_FILENO) close(fd_in);

    return ecmd;
}



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

        cmd_2_t cmd;
        const char* curs = line;
        //printf("begin parse\n");
        int pst = parse_cmd_2(&curs, &cmd);

        //printf("parse_st = %d\n", pst);
        if (pst) {
            printf("Syntax error\n");
            continue;
        }
        pp_cmd_2(stdout, &cmd);
        printf("\n");

        printf("\033d");

        ecmd_2_t* ecmd = exec_cmd_2(&cmd);
        ecmd->num  = next_cmd_num++;
        ecmd->next = ecmd_llist;
        ecmd_llist = ecmd;
        run_fg();

        destr_cmd_2(&cmd);
    }
}
