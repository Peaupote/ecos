#include <stdbool.h>
#include <stdint.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <headers/display.h>
#include <headers/keyboard.h>

struct display_info di;
int fdisplay; // file desc to FILE_DISPLAY

#define MARGIN 10

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
        fprintf(stderr, "can't open %s\n", fname);
        return false;
    }

    int rc = write(fd, buf->buf, buf->cur1 - buf->buf);
    if (rc < 0) {
        close(fd);
        return false;
    }

    rc = write(fd, buf->cur2, buf->size - (buf->cur2 - buf->buf));
    close(fd);

    printf("file %s saved !\n", fname);

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
        fprintf(stderr, "file %s too big\n", fname);
        exit(1);
    }

    lseek(fd, 0, SEEK_SET);
    read(fd, b->buf + b->size - size, b->size - size);

    b->cur1 = b->buf;
    b->cur2 = b->buf + (b->size - size - 1);

    close(fd);

    return b;
}

// TODO real print
void display(buf_t *buf) {
    printf("PRINT BUFFER\n");
    for (size_t i = 0; i < buf->size; ++i) {
        char *ptr = buf->buf + i;
        if (ptr < buf->cur1 || ptr > buf->cur2) fputc(buf->buf[i], stdout);
        else if (ptr == buf->cur1) fputc('*', stdout);
    }
    printf("\nEND\n");
}

int main(int argc, char *argv[]) {
    fdisplay = open(DISPLAY_FILE, O_RDWR, 0640);
    if (fdisplay < 0) {
        perror("open display");
        return 1;
    }

    int rc = read(fdisplay, &di, sizeof(di));
    if (rc < 0) {
        perror("read display");
        return 1;
    }

    // switch to live mode
    printf("\033l\n", stdout);

    buf_t *b;
    if (argc <= 1) b = create(8);
    else b = open_buf(argv[1]);

    display(b);

    for (;;) {
        key_event ev;
        rc = read(0, &ev, sizeof(key_event));
        if (rc < 0) {
            perror("read");
            return 1;
        }

        if ((ev.flags & (KEY_FLAG_MODS | KEY_FLAG_PRESSED))
            == (KEY_FLAG_CTRL | KEY_FLAG_PRESSED)) {
            switch(ev.ascii) {
            case 's': save(b, argc > 1 ? argv[1] : "file"); break;
            }

        } else if (ev.flags & KEY_FLAG_PRESSED) {
            switch (ev.key) {
            case KEY_BACKSPACE:   rm(b);   break;
            case KEY_LEFT_ARROW:  left(b); break;
            case KEY_RIGHT_ARROW: right(b);break;
            case KEY_ENTER:       put(&b, '\n'); break;
            default:
                if (ev.ascii >= 20 && ev.ascii < 127)
                    put(&b, ev.ascii);
                break;
            }

            display(b);
        }
    }

}
