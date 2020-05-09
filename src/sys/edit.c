#include <stdbool.h>
#include <stdint.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <headers/tty.h>
#include <headers/display.h>
#include <headers/keyboard.h>

enum msg_kind {
    INFO,
    ERR
};

void print_msg(const char *msg, enum msg_kind kind);

/**
 * Gap buffer
 * Some text before the cursor [   (gap)   ] and some after
 * ^ buf                        ^ cur1    ^ cur2
 */
typedef struct {
    char *cur1, *cur2;
    size_t size;
    char   buf[];
} buf_t;

buf_t *create(size_t size) {
    buf_t *b = malloc(offsetof(buf_t, buf) + size);
    if (b) {
        b->size = size;
        b->cur1 = b->buf;
        b->cur2 = b->buf + size - 1;
    }

    return b;
}

void put(buf_t **b, char c) {
    if ((*b)->cur1 == (*b)->cur2) { // realloc in a twice bigger buffer
        buf_t *n = create((*b)->size << 1);

        size_t off1 = (*b)->cur1 - (*b)->buf, off2 = (*b)->cur2 - (*b)->buf;
        memcpy(n->buf, (*b)->buf, off1);

        size_t sz = (*b)->size - off2;
        memcpy(n->buf + (n->size - sz), (*b)->cur2, sz);

        n->cur1 = n->buf + off1;
        n->cur2 = n->buf + n->size - sz - 1;

        free(*b);
        *b = n;
    }

    *(*b)->cur1++ = c;
}

void rm(buf_t *b) {
    if (b->cur1 > b->buf) --b->cur1;
}

void left(buf_t *b) {
    if (b->cur1 > b->buf) {
        b->cur2[0] = b->cur1[-1];
        --b->cur1;
        --b->cur2;
    }
}

void right(buf_t *b) {
    if (b->cur2 + 1 < b->buf + b->size) {
        b->cur1[0] = b->cur2[1];
        ++b->cur1;
        ++b->cur2;
    }
}

bool save(buf_t *buf, const char *fname) {
    int fd = open(fname, O_WRONLY|O_TRUNC|O_CREAT);
    if (fd < 0) {
        char msg[256];
        sprintf(msg, "can't open %s\n", fname);
        print_msg(msg, ERR);
        return false;
    }

    int rc = write(fd, buf->buf, buf->cur1 - buf->buf);
    if (rc < 0) {
        close(fd);
        return false;
    }

    rc = write(fd, buf->cur2, buf->size - (buf->cur2 - buf->buf));
    close(fd);

    print_msg("file saved !\n", INFO);

    return rc >= 0;
}

buf_t *open_buf(const char *fname) {
    int fd = open(fname, O_RDONLY);
    if (fd < 0) {
        return create(1024);
    }

    int size = lseek(fd, 0, SEEK_END);
    buf_t *b = create(size << 2); // make buffer twice as big as the file
    if (!b) {
        char msg[256];
        sprintf(msg, "file %s too big", fname);
        print_msg(msg, ERR);
        exit(42);
        return 0;
    }

    lseek(fd, 0, SEEK_SET);
    read(fd, b->buf + b->size - size, b->size - size);

    b->cur1 = b->buf;
    b->cur2 = b->buf + (b->size - size - 1);

    close(fd);

    return b;
}

/**
 * Visual
 */

typedef struct screen {
    buf_t *buf;
    char *fname;

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

screen_t *fst_screen = 0; // first screen in linked list
screen_t *curr;       // current screen
int msg_col;

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
    s->line = 0;
    s->col = 0;

    s->pv = 0;
    s->nx = fst_screen;
    if (fst_screen) fst_screen->pv = s;
    fst_screen = s;
    curr = s;
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

void display();

void print_msg(const char *msg, enum msg_kind kind) {
    cursor_at(msg_col, H-1);

    switch (kind) {
    case INFO: printf("\033[0m"); break;
    case ERR : printf("\033[31m error: "); break;
    }

    unsigned int w = printf("%s", msg);
    while (w < W) fputc(' ', stdout), ++w;
    fflush(stdout);
}

void print_bar() {
    cursor_at(0, H-1);
    msg_col = printf("buffer: ");
    if (curr->fname) msg_col += printf("%s ", curr->fname);
    else msg_col += printf("<no file> ");
    fflush(stdout);

    char buf[256];
    int sz = sprintf(buf, "%d:%d (size %d)", curr->line, curr->col,
                     curr->buf->size - (curr->buf->cur2 - curr->buf->cur1 + 1));
    cursor_at(W - sz - 1, H-1);
    write(STDOUT_FILENO, buf, sz);
}

// TODO fast clean
void clean() {
    cursor_at(0, 0);

    printf("\033[0m");
    for (unsigned int i = 0; i < H; ++i) {
        for (unsigned int j = 0; j < W; ++j) {
            fputc(' ', stdout);
        }
    }
    fflush(stdout);
}

void display() {
    clean();
    cursor_at(0, 0);

    buf_t *buf = curr->buf;
    unsigned int line = 1, col = 0, off = 0;
    for (size_t i = 0; i < buf->size; ++i) {
        char *ptr = buf->buf + i;
        if (ptr < buf->cur1 || ptr > buf->cur2) {
            if (buf->buf[i] == '\n') {
                fflush(stdout);
                cursor_at(0, off + line++);
                col = 0;
            }
            else {
                if (++col == W+1) ++off;
                fputc(buf->buf[i], stdout);
            }
        } else if (ptr == buf->cur1) {
            curr->line = line;
            curr->col = col;
            printf("*");
        }
    }

    fflush(stdout);

    print_bar();
}

bool get_live(tty_live_t* rt) {
    int c;
    while ((c = fgetc(stdin)) != TTY_LIVE_MAGIC)
        if (c == EOF) return false;
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

    // switch to live mode
    printf("\033l\n", stdout);

    for (int i = 1; i < argc; ++i) {
        open_screen(argv[i]);
    }

    if (argc == 1) open_screen(0);

    display();

    while (get_live(&lt)) {
        key_event *ev = &lt.ev;

        if ((ev->flags & (KEY_FLAG_MODS | KEY_FLAG_PRESSED))
            == (KEY_FLAG_CTRL | KEY_FLAG_PRESSED)) {
            switch(ev->ascii) {
            case 's':
                // TODO ask filename
                if (curr->fname) save(curr->buf, curr->fname);
                break;
            case 'n': open_screen(0); break;
            // TODO case 'o': break; open fname
            case 'q': exit(0); break;

            default: // TODO clean switch
                switch (ev->key) {
                case KEY_LEFT_ARROW: next_screen(); break;
                case KEY_RIGHT_ARROW: prev_screen(); break;
                }
            }

            display();
        } else if (ev->flags & KEY_FLAG_PRESSED) {
            switch (ev->key) {
            case KEY_BACKSPACE:   rm(curr->buf);   break;
            case KEY_LEFT_ARROW:  left(curr->buf); break;
            case KEY_RIGHT_ARROW: right(curr->buf);break;
            case KEY_ENTER:       put(&curr->buf, '\n'); break;
            default:
                if (ev->ascii >= 20 && ev->ascii < 127)
                    put(&curr->buf, ev->ascii);
                break;
            }

            display();
        }
    }

    return 0;
}
