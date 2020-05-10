#include <stdbool.h>
#include <stdint.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <headers/tty.h>
#include <headers/display.h>
#include <headers/keyboard.h>

enum edit_mode {
    BUFFER,
    QUERY
};

enum bar_status {
    NONE,
    INFO,
    ERR
};

void display();
void make_query(const char *query, void (*callback)(char *));
void print_msg(const char *msg, enum bar_status st);

/**
 * Gap buffer
 * Some text before the cursor [   (gap)   ] and some after
 * ^ buf                        ^ cur1    ^ cur2
 */
typedef struct {
    char *cur1, *cur2;
    int saved;
    size_t size;
    char   buf[];
} buf_t;


/**
 * Visual
 */

typedef struct screen {
    buf_t *buf;
    char *fname;

    unsigned int nline;
    unsigned int line;
    unsigned int col;

    struct screen *nx, *pv;
} screen_t;

unsigned int W, H;

void cursor_at(int l, int c) {
    char cmd[50];
    int ccmd = sprintf(cmd, "\033c%u;%u;", l, c);
    write(STDOUT_FILENO, cmd, ccmd);
}

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
        b->saved = 0;
        b->size  = size;
        b->cur1  = b->buf;
        b->cur2  = b->buf + size - 1;
    }

    return b;
}

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

void empty_buffer(buf_t *buf) {
    buf->cur1 = buf->buf;
    buf->cur2 = buf->buf + buf->size - 1;
}

char put(buf_t **b, char c) {
    if ((*b)->cur1 == (*b)->cur2) { // realloc in a twice bigger buffer
        buf_t *n = create((*b)->size << 1);
        if (!n) {
            print_msg("can't alloc bigger buffer", ERR);
            exit(42);
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
    return c;
}

char rm(buf_t *b) {
    if (b->cur1 > b->buf) {
        b->saved = 0;
        return *b->cur1--;
    }

    return 0;
}

char left(buf_t *b) {
    if (b->cur1 > b->buf) {
        char r = b->cur1[-1];
        b->cur2[0] = r;
        --b->cur1;
        --b->cur2;
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
        return r;
    }

    return 0;
}

void save(char *fname) {
    int fd = open(fname, O_WRONLY|O_TRUNC|O_CREAT);
    if (fd < 0) {
        char msg[256];
        sprintf(msg, "can't open %s\n", fname);
        print_msg(msg, ERR);
        return;
    }

    buf_t *buf = curr->buf;
    int rc = write(fd, buf->buf, buf->cur1 - buf->buf);
    if (rc < 0) {
        close(fd);
        return;
    }

    rc = write(fd, buf->cur2+1, buf->size - (buf->cur2 - buf->buf));
    if (rc < 0) print_msg("error while saving", ERR);
    else print_msg("file saved !\n", INFO);
    close(fd);

    buf->saved = 1;
    if (curr->fname) free(curr->fname);
    curr->fname = malloc(strlen(fname)+1);
    strcpy(curr->fname, fname);
}

void save_and_close(char *fname) {
    save(fname);
    close_current_screen();
}

void save_close(char *ans) {
    if (!strcmp("yes" , ans)) {
        if (curr->fname) save_and_close(curr->fname);
        else make_query("filename ? ", &save_and_close);
    } else if (!strcmp("no", ans)) close_current_screen();
    else print_msg("please answer by yes or no", ERR);
}

buf_t *open_buf(const char *fname) {
    int fd = open(fname, O_RDONLY);
    if (fd < 0) {
        return create(1024);
    }

    int size = lseek(fd, 0, SEEK_END);
    buf_t *b = create(size << 1); // make buffer twice as big as the file
    if (!b) {
        char msg[256];
        sprintf(msg, "file %s too big", fname);
        print_msg(msg, ERR);
        return 0;
    }

    lseek(fd, 0, SEEK_SET);
    int rc = read(fd, b->buf + size, size);
    if (rc < size) {
        free(b);
        print_msg("can't read file", ERR);
        return 0;
    }

    b->cur1 = b->buf;
    b->cur2 = b->buf + size - 1;

    close(fd);

    return b;
}

// --- visual

void open_screen(char *fname) {
    buf_t *b = fname ? open_buf(fname) : create(1024);
    screen_t *s = malloc(sizeof(screen_t));
    if (!s) {
        free(b);
        print_msg("can't open a new screen", ERR);
        return;
    }

    if (fname) {
        s->fname = malloc(strlen(fname) + 1);
        memcpy(s->fname, fname, strlen(fname) + 1);
    } else s->fname = 0;

    s->buf = b;
    s->nline = 0;
    s->line = 0;
    s->col = 0;

    s->pv = 0;
    s->nx = fst_screen;
    if (fst_screen) fst_screen->pv = s;
    fst_screen = s;
    curr = s;

    char buf[256];
    if (fname) sprintf(buf, "open %s in new buffer", fname);
    else sprintf(buf, "open a new empty buffer");
    print_msg(buf, INFO);
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
}

void prev_screen() {
    if (!curr->pv) {
        print_msg("no buffer on the left", INFO);
        return;
    }

    curr = curr->pv;
}

void next_screen() {
    if (!curr->nx) {
        print_msg("no buffer on the right", INFO);
        return;
    }

    curr = curr->nx;
}

char next_line() {
    if (curr->line == curr->nline) return 0;

    buf_t *b = curr->buf;
    int col = curr->col;

    char c;
    while (c = right(b), c != '\n' && c != 0);
    for (int i = 0; i < col; ++i) {
        c = right(b);
        if (c == '\n') {
            left(b);
            break;
        }
    }

    return 1;
}

char prev_line() {
    if (curr->line <= 1) return 0;

    buf_t *b = curr->buf;
    int col = curr->col;

    char c;
    while (c = left(b), c != '\n' && c != 0);
    while (c = left(b), c != '\n' && c != 0);

    if (c != 0) right(b);
    for (int i = 0; i < col; ++i) {
        c = right(b);
        if (c == '\n') {
            left(b);
            break;
        }
    }

    return 1;
}


void print_msg(const char *new_msg, enum bar_status st) {
    bar_status = st;
    msg_count  = 1;
    strcpy(msg, new_msg);
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

    char buf[256];
    int sz = sprintf(buf, "%d:%d (size %d)", curr->line, curr->col,
                     curr->buf->size - (curr->buf->cur2 - curr->buf->cur1 + 1));
    cursor_at(W - sz - 1, H-1);
    write(STDOUT_FILENO, buf, sz);

    printf("\033[0m"); fflush(stdout);
}

void print_query_bar() {
    cursor_at(0, H-1);
    printf("\033[47;30m");
    for (unsigned int i = 0; i < W; ++i) fputc(' ', stdout);
    fflush(stdout);

    cursor_at(0, H-1);
    fputs(query, stdout);
    for (size_t i = 0; i < input->size; ++i) {
        char *ptr = input->buf + i;
        if (ptr < input->cur1 || ptr > input->cur2) {
            fputc(*ptr, stdout);
            if (ptr == input->cur2+1) fputs("\033[47;30m", stdout);
        } else if (ptr == input->cur1) fputs("\033[0m", stdout);
    }

    if (msg_count > 0) {
        fputs("    ", stdout);
        fputs(msg, stdout);
        if (--msg_count == 0) bar_status = NONE;
    }

    fputs(" \033[0m", stdout);
    fflush(stdout);
}

void clean() {
    write(STDOUT_FILENO, "\033x", 2);
}

void display_buffer() {
    clean(); // TODO update

    // print buffer
    cursor_at(0, 0);
    buf_t *buf = curr->buf;
    unsigned int line = 1, col = 0, off = 0;
    for (size_t i = 0; i < buf->size; ++i) {
        char *ptr = buf->buf + i;
        if (ptr < buf->cur1 || ptr > buf->cur2) {
            if (buf->buf[i] == '\n') {
                if (ptr == buf->cur2 + 1) printf(" \033[0m");
                fflush(stdout);
                cursor_at(0, off + line++);
                col = 0;
            }
            else {
                if (++col == W+1) ++off;
                fputc(buf->buf[i], stdout);
                if (ptr == buf->cur2 + 1) printf("\033[0m");
            }
        } else if (ptr == buf->cur1) {
            curr->line = line;
            curr->col = col;
            printf("\033[47;30m");
        }
    }

    printf(" \033[0m");

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

static inline void switch_back() {
    write(STDOUT_FILENO, "\033l", 2);
    clean();
    display();
}

bool get_live(tty_live_t* rt) {
    int c;
again:
    while ((c = fgetc(stdin)) != TTY_LIVE_MAGIC) {
        if (c == EOF) {
            if (errno == EINTR) {
                switch_back();
                goto again;
            }

            return false;
        }
    }

    char* buf = (char*)rt;
    buf[0] = TTY_LIVE_MAGIC;
    for (size_t i = 1; i < sizeof(tty_live_t); ++i) {
        if ((c = fgetc(stdin)) == EOF) {
            if (errno == EINTR) {
                switch_back();
                goto again;
            }

            return false;
        }
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
            == (KEY_FLAG_CTRL | KEY_FLAG_PRESSED)) {
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
                }
            }

            display();
        } else if (ev->flags & KEY_FLAG_PRESSED) {
            char c = 0;
            buf_t **t = mode == BUFFER ? &curr->buf : &input;

            switch (ev->key) {
            case KEY_BACKSPACE:
                if (mode == QUERY && is_empty_buffer(input)) mode = BUFFER;
                else c = rm(*t);
                break;

            case KEY_LEFT_ARROW:  c = left(*t); break;
            case KEY_RIGHT_ARROW: c = right(*t);break;
            case KEY_DOWN_ARROW:  c = next_line(); break;
            case KEY_UP_ARROW:    c = prev_line(); break;
            case KEY_ENTER:
                switch (mode) {
                case BUFFER: c = put(t, '\n'); break;
                case QUERY:
                    buffer_to_string(input, query);
                    mode = BUFFER;
                    c = 1;
                    query_callback(query);
                    break;
                }
                break;

            default:
                if (ev->ascii >= 20 && ev->ascii < 127)
                    c = put(t, ev->ascii);
                break;
            }

            if (c) display();
        }
    }

    return 0;
}
