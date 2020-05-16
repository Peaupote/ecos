#include <stdbool.h>
#include <stdint.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <headers/tty.h>
#include <headers/display.h>
#include <headers/keyboard.h>

// which buffer user writes to
enum edit_mode {
    BUFFER, // buffer containing edited file
    QUERY   // buffer answering a query
};

// define color of status bar
enum bar_status {
    NONE,
    INFO,
    ERR
};

void display();
void make_query(const char *query, void (*callback)(char *));
void print_msg(enum bar_status st, const char *fmt, ...);

static inline void clean() {
    write(STDOUT_FILENO, "\033x", 2);
}

/**
 * Gap buffer
 * Some text before the cursor [   (gap)   ] and some after
 * ^ buf                        ^ cur1    ^ cur2
 */
typedef struct {
    char *cur1, *cur2;

    // position of cursor in the text
    unsigned int line;
    unsigned int col;
    unsigned int nline;

    // has the file been saved since last update
    unsigned int saved;

    // gap buffer
    size_t size;
    char   buf[];
} buf_t;


/**
 * Visual
 */

typedef struct screen {
    buf_t *buf;
    char *fname;

    // first line of buffer visible on screen
    char *fst_line;
    unsigned int fst_line_nb;

    struct screen *nx, *pv;
} screen_t;

// screen width and height
unsigned int W, H;

void cursor_at(int l, int c) {
    char cmd[50];
    int ccmd = sprintf(cmd, "\033c%u;%u;", l, c);
    write(STDOUT_FILENO, cmd, ccmd);
}

// global variables

enum edit_mode mode = BUFFER;

char query[256];                    // question asked
buf_t *input;                       // buffer were to write answer
void (*query_callback)(char *) = 0; // function to call after press enter

screen_t *fst_screen = 0; // first screen in linked list
screen_t *curr;       // current screen
char msg[256];
enum bar_status bar_status;
int msg_count = 0;

void close_current_screen();

// ------ buffer

buf_t *create(size_t size) {
    buf_t *b = malloc(offsetof(buf_t, buf) + size);
    if (b) {
        b->line  = 1;
        b->nline = 1;
        b->col   = 0;
        b->saved = 1;
        b->size  = size;
        b->cur1  = b->buf;
        b->cur2  = b->buf + size - 1;
    }

    return b;
}

// write buffer content to string
void buffer_to_string(buf_t *buf, char *dst) {
    for (size_t i = 0; i < buf->size; ++i) {
        char *ptr = buf->buf + i;
        if (ptr < buf->cur1 || ptr > buf->cur2) *dst++ = *ptr;
    }

    *dst = 0;
}

int is_empty_buffer(buf_t *buf) {
    return buf->cur1 == buf->buf && buf->cur2 == buf->buf + buf->size - 1;
}

// remove content from buffer
void empty_buffer(buf_t *buf) {
    buf->line = 1;
    buf->cur1 = buf->buf;
    buf->cur2 = buf->buf + buf->size - 1;
}

// write to buffer
char put(buf_t **b, char c) {
    if ((*b)->cur1 == (*b)->cur2) { // realloc in a twice bigger buffer
        buf_t *n = create((*b)->size << 1);
        if (!n) {
            print_msg(ERR, "can't alloc bigger buffer");
            return 0;
        }

        /**
         * xxxxxxxxx|xxxxxxxx
         * ^ buf    ^ cur1 = cur2
         */

        size_t off = (*b)->cur1 - (*b)->buf;
        memcpy(n->buf, (*b)->buf, off);

        size_t sz = (*b)->size - off - 1;
        memcpy(n->buf + (n->size - sz), (*b)->cur2+1, sz);

        n->cur1 = n->buf + off;
        n->cur2 = n->buf + n->size - sz - 1;

        free(*b);
        *b = n;
    }

    (*b)->saved = 0;
    *(*b)->cur1++ = c;

    if (c == '\n') ++(*b)->nline, ++(*b)->line, (*b)->col = 0;
    else ++(*b)->col;
    return c;
}

char rm(buf_t *b) {
    if (b->cur1 > b->buf) {
        b->saved = 0;
        char c = *b->cur1--;

        if (c == '\n') {
            b->col = 0;
            for (char *p = b->cur1; p > b->buf && *p != '\n'; --p, ++b->col);
            --b->nline;
            --b->line;
        }
        return c;
    }

    return 0;
}

// move cursor

char left(buf_t *b) {
    if (b->cur1 > b->buf) {
        char r = b->cur1[-1];
        b->cur2[0] = r;
        --b->cur1;
        --b->cur2;

        if (r == '\n') {
            b->col = 0;
            for (char *p = b->cur1-1; p >= b->buf && *p != '\n'; --p, ++b->col);
            --b->line;
        } else --b->col;
        return r;
    }

    return 0;
}

char right(buf_t *b) {
    if (b->cur2 + 1 < b->buf + b->size) {
        char r = b->cur2[1];
        b->cur1[0] = r;
        ++b->cur1;
        ++b->cur2;

        if (r == '\n') {
            b->col = 0;
            ++b->line;
        } else ++b->col;
        return r;
    }

    return 0;
}

// save buffer to file
void save(char *fname) {
    int fd = open(fname, O_WRONLY|O_TRUNC|O_CREAT);
    if (fd < 0) {
        print_msg(ERR, "can't open %s\n", fname);
        return;
    }

    buf_t *buf = curr->buf;
    int rc = write(fd, buf->buf, buf->cur1 - buf->buf);
    if (rc < 0) {
        close(fd);
        return;
    }

    rc = write(fd, buf->cur2+1, buf->size - (buf->cur2 - buf->buf) - 1);
    if (rc < 0) print_msg(ERR, "error while saving %s", fname);
    close(fd);

    buf->saved = 1;
    char *pv = curr->fname;
    curr->fname = strdup(fname);
    free(pv);

    print_msg(INFO, "%s saved !", curr->fname);
}

// callback functions for save query

void save_and_close(char *fname) {
    save(fname);
    close_current_screen();
}

void save_close(char *ans) {
    if (!strcmp("yes" , ans)) {
        if (curr->fname) save_and_close(curr->fname);
        else make_query("filename ? ", &save_and_close);
    } else if (!strcmp("no", ans)) close_current_screen();
    else print_msg(ERR, "please answer by yes or no");
}

// read file into buffer
buf_t *open_buf(const char *fname) {
    int fd = open(fname, O_RDONLY);
    if (fd < 0) {
        return create(1024);
    }

    int size = lseek(fd, 0, SEEK_END);
    buf_t *b = create(size << 1); // make buffer twice as big as the file
    if (!b) {
        print_msg(ERR, "file %s too big", fname);
        return 0;
    }

    lseek(fd, 0, SEEK_SET);
    int rc = read(fd, b->buf + size, size);
    if (rc < size) {
        free(b);
        print_msg(ERR, "can't read file %s", fname);
        return 0;
    }

    b->nline = 1;
    for (int i = 0; i < size; ++i)
        if (b->buf[size + i] == '\n') ++b->nline;

    b->cur1 = b->buf;
    b->cur2 = b->buf + size - 1;

    close(fd);

    return b;
}

// move cursor

char next_line(buf_t *buf) {
    if (buf->line == buf->nline) return 0;

    char c;
    unsigned int col = buf->col;
    while (c = right(buf), c != '\n' && c != 0);
    for (unsigned int i = 0; i < col; ++i) {
        c = right(buf);
        if (c == '\n') {
            left(buf);
            break;
        }
    }

    return 1;
}

char prev_line(buf_t *buf) {
    if (buf->line <= 1) return 0;

    char c;
    unsigned int col = buf->col;
    while (c = left(buf), c != '\n' && c != 0);
    while (c = left(buf), c != '\n' && c != 0);

    if (c != 0) right(buf);
    for (unsigned int i = 0; i < col; ++i) {
        c = right(buf);
        if (c == '\n') {
            left(buf);
            break;
        }
    }

    return 1;
}


// --- screen

void open_screen(char *fname) {
    buf_t *b = fname ? open_buf(fname) : create(1024);
    screen_t *s = malloc(sizeof(screen_t));
    if (!s) {
        free(b);
        print_msg(ERR, "can't open a new screen");
        return;
    }

    if (fname) {
        s->fname = malloc(strlen(fname) + 1);
        memcpy(s->fname, fname, strlen(fname) + 1);
    } else s->fname = 0;

    s->buf = b;
    s->fst_line = b->buf;
    s->fst_line_nb = 1;

    s->pv = 0;
    s->nx = fst_screen;
    if (fst_screen) fst_screen->pv = s;
    fst_screen = s;
    curr = s;

    clean();

    if (fname) print_msg(INFO, "open %s in new buffer", fname);
    else print_msg(INFO, "open a new empty buffer");
}

void close_current_screen() {
    // if only screen then exit
    if (!curr->nx && !curr->pv) exit(0);

    screen_t *s = curr;
    if (curr->pv) {
        curr = curr->pv;
        curr->nx = s->nx;
    } else {
        curr = curr->nx;
        curr->pv = s->pv;
    }

    free(s->buf);
    free(s->fname);
    free(s);
    clean();
}

void prev_screen() {
    if (!curr->pv) {
        print_msg(INFO, "no buffer on the left");
        return;
    }

    curr = curr->pv;
    clean();
}

void next_screen() {
    if (!curr->nx) {
        print_msg(INFO, "no buffer on the right");
        return;
    }

    curr = curr->nx;
    clean();
}

void screen_up() {
    if (curr->fst_line_nb == 1) {
        print_msg(INFO, "top of file");
        return;
    }

    buf_t *buf = curr->buf;

    --curr->fst_line_nb;
    for(curr->fst_line -= 2;
        curr->fst_line > buf->buf && *curr->fst_line != '\n';
        --curr->fst_line);
    if (*curr->fst_line == '\n') ++curr->fst_line;
}

void screen_down() {
    if (curr->fst_line_nb + H - 3 == curr->buf->nline) {
        print_msg(INFO, "end of file");
        return;
    }

    buf_t *buf = curr->buf;
    ++curr->fst_line_nb;
    while (curr->fst_line < buf->buf + buf->size && *curr->fst_line != '\n')
        curr->fst_line++;
    if (*curr->fst_line == '\n') ++curr->fst_line;
}

// --- visual

void print_msg(enum bar_status st, const char *fmt, ...) {
    va_list params;
    va_start(params, fmt);
    bar_status = st;
    msg_count  = 1;
    vsprintf(msg, fmt, params);
    va_end(params);
}

void print_bar() {
    cursor_at(0, H-1);
    switch (bar_status) {
    case NONE:
    case INFO: fputs("\033[47;30m", stdout); break;
    case ERR:  fputs("\033[41;37m", stdout); break;
    }

    for (unsigned int i = 0; i < W; ++i) fputc(' ', stdout);
    fflush(stdout);

    cursor_at(0, H-1);
    fputs("buffer: ", stdout);
    if (curr->fname) fputs(curr->fname, stdout);
    else fputs("<no file>", stdout);

    if (msg_count > 0) {
        fputs("    ", stdout);
        fputs(msg, stdout);
        if (--msg_count == 0) bar_status = NONE;
    }

    fflush(stdout);

    buf_t *cb = curr->buf;

    char buf[256];
    int sz =
        sprintf(buf, "%d:%d (size %d)", cb->line, cb->col,
                cb->size - (cb->cur2 - cb->cur1 + 1));
    cursor_at(W - sz - 1, H-1);
    write(STDOUT_FILENO, buf, sz);

    printf("\033[0m"); fflush(stdout);
}

void print_query_bar() {
    cursor_at(0, H-1);
    fputs("\033[47;30m", stdout);

    unsigned int col = strlen(query)+1;
    fputs(query, stdout);
    char *p = input->buf, *endp = input->buf + input->size;
    while (col++ < W-1 && p < endp) {
        if (p < input->cur1 || p > input->cur2) {
            fputc(*p, stdout);
            if (p == input->cur2 + 1) fputs("\033[47;30m", stdout);
            ++p;
        } else if (p == input->cur1) {
            fputs("\033[0m", stdout);
            p = input->cur2 + 1;
        }
    }

    if (msg_count > 0) {
        col += fputs("    ", stdout);
        col += fputs(msg, stdout);
        if (--msg_count == 0) bar_status = NONE;
    }

    fputs(" \033[47;30m", stdout);
    while(col++ < W-1) fputc(' ', stdout);
    fputs("\033[0m", stdout);
    fflush(stdout);
}

void display_buffer() {
    cursor_at(0, 0);
    buf_t *buf = curr->buf;

    // adjust scrolling
    if (buf->line < curr->fst_line_nb) screen_up();
    else if (buf->line > curr->fst_line_nb + H-3) screen_down();

    // print buffer
    unsigned int line = 1, col = 0, off = 0;
    fputs("\033[0m", stdout);
    char *p = curr->fst_line, *endp = buf->buf + buf->size;
    while (line < H-1 && p < endp) {
        if (p < buf->cur1 || p > buf->cur2) {
            if (*p == '\n') {
                fputs(" \033[0m", stdout);
                while (col < W) fputc(' ', stdout), ++col; // clean line
                fflush(stdout);
                cursor_at(0, line++ + off);
                col = 0;
            } else {
                if (col++ == W) ++off;
                if (*p == '\t') fputs("    ", stdout);
                else fputc(*p, stdout);
                if (p == buf->cur2 + 1) fputs("\033[0m", stdout);
            }

            ++p;
        } else if (p == buf->cur1) {
            fputs("\033[47;30m", stdout);
            p = buf->cur2 + 1;
        }
    }

    fputs(" \033[0m", stdout);

    // clean last 2 lines (in case one was deleted)
    while (++col < 2 * W) fputc(' ', stdout);
    fflush(stdout);
}

void display() {
    switch (mode) {
    case BUFFER:
        display_buffer();
        print_bar();
        break;

    case QUERY:
        print_query_bar();
        break;
    }
}

void make_query(const char *q, void (*callback)(char*)) {
    mode = QUERY;
    query_callback = callback;
    strcpy(query, q);
    empty_buffer(input);
}

// --

void sighdl(int signum) {
    int sv_errno = errno;
    if (signum == SIGTSTP) {
        write(STDIN_FILENO, "\033d", 2);
        kill(getpid(), SIGSTOP);
    } else if (signum == SIGCONT) {
        write(STDOUT_FILENO, "\033l", 2);
        clean();
        display();
    }
    errno = sv_errno;
}

// read inputs from live tty
bool get_live(tty_live_t* rt) {
    int c;
    while ((c = fgetc(stdin)) != TTY_LIVE_MAGIC) {
        if (c == EOF) return false;
    }

    char* buf = (char*)rt;
    buf[0] = TTY_LIVE_MAGIC;
    for (size_t i = 1; i < sizeof(tty_live_t); ++i) {
        if ((c = fgetc(stdin)) == EOF) return false;
        buf[i] = c;
    }

    return true;
}

int main(int argc, char *argv[]) {
    // get tty info
    tty_live_t lt;

    write(STDOUT_FILENO, "\033i", 2);

    do {
        if (!get_live(&lt)) return 1;
    } while (lt.ev.key || lt.ev.ascii != 'i');

    while (get_live(&lt) && !lt.ev.key && lt.ev.ascii != ';')
        W = W * 10 + lt.ev.ascii - '0';
    while (get_live(&lt) && !lt.ev.key && lt.ev.ascii != ';')
        H = H * 10 + lt.ev.ascii - '0';

    signal(SIGTSTP, sighdl);
    signal(SIGCONT, sighdl);
    // switch to live mode
    write(STDOUT_FILENO, "\033l", 2);

    // alloc command input buffer
    input = create(16);

    for (int i = 1; i < argc; ++i) {
        open_screen(argv[i]);
    }

    if (argc == 1) open_screen(0);

    clean();
    display();

    while (get_live(&lt)) {
        key_event *ev = &lt.ev;

        if ((ev->flags & (KEY_FLAG_MODS | KEY_FLAG_PRESSED))
            == (KEY_FLAG_CTRL | KEY_FLAG_PRESSED)) { // commands
            switch(ev->ascii) {
            case 's':
                if (!curr->fname) make_query("save to file ? ", &save);
                else save(curr->fname);
                break;
            case 'n': open_screen(0); break;
            case 'o': make_query("open file ? ", &open_screen); break;
            case 'k':
                if (!curr->buf->saved)
                    make_query("save before closing ? (yes/no) ", save_close);
                else close_current_screen();
                break;
            case 'q': exit(0); break;

            default:
                switch (ev->key) {
                case KEY_RIGHT_ARROW: next_screen(); break;
                case KEY_LEFT_ARROW:  prev_screen(); break;
                case KEY_UP_ARROW:    screen_up(); break;
                case KEY_DOWN_ARROW:  screen_down(); break;
                }
            }

            display();
        } else if (ev->flags & KEY_FLAG_PRESSED) { // action on current buffer
            buf_t **t = mode == BUFFER ? &curr->buf : &input;

            switch (ev->key) {
            case KEY_BACKSPACE:
                if (mode == QUERY && is_empty_buffer(input)) mode = BUFFER;
                else rm(*t);
                break;

            case KEY_LEFT_ARROW:  left(*t); break;
            case KEY_RIGHT_ARROW: right(*t);break;
            case KEY_DOWN_ARROW:  next_line(*t); break;
            case KEY_UP_ARROW:    prev_line(*t); break;
            case KEY_ENTER:
                switch (mode) {
                case BUFFER: put(t, '\n'); break;
                case QUERY:
                    buffer_to_string(input, query);
                    mode = BUFFER;
                    query_callback(query);
                    break;
                }
                break;

            case KEY_TAB: put(t, '\t'); break;
            default:
                if (ev->ascii >= 20 && ev->ascii < 127)
                    put(t, ev->ascii);
                break;
            }

            display();
        }
    }

    return 0;
}
