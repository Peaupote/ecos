#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>

#define C 1
#define W 2
#define L 4

int flags;

void read_flags(const char *p) {
    switch (*++p) {
    case 'c': flags |= C; break;
    case 'w': flags |= W; break;
    case 'l': flags |= L; break;
    default:
        printf("unkown flag %d", *p);
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    char *files[256] = { 0 }, **ptr = files;

    for (int i = 1; i < argc; ++i) {
        if (*argv[i] == '-') read_flags(argv[i]);
        else *ptr++ = argv[i];
    }

    if (argc == 1 || !flags) {
        flags = C | W | L;
    }

    int c = 0, w = 0, l = 0;
    int fd, rc;

    if (argc == 1) {
        fd = 0;
        goto start;
    }

#define SZ 1024
    char buf[SZ];

    for (ptr = files; *ptr; ++ptr) {
        fd = open(*ptr, O_RDONLY);
        if (fd < 0) {
            perror("wc");
            exit(1);
        }
    start:
        l = 0;
        w = 0;
        c = 0;

        int inw = 0;
        while ((rc = read(fd, buf, SZ)) > 0) {
            c += rc;
            for (int i = 0; i < rc; ++i) {
                switch (buf[i]) {
                case '\n': ++l;
                    // fall through
                case ' ': if(inw) ++w, inw = 0; break;
                default: inw = 1; break;
                }
            }
        }

        if (rc < 0) {
            perror("wc: read");
            close(fd);
            exit(1);
        }

        if (inw)   ++w;

        if (flags&L) printf("%d\t", l);
        if (flags&W) printf("%d\t", w);
        if (flags&C) printf("%d\t", c);
        if (*ptr) printf("%s\n", *ptr);

        close(fd);
    }

    return 0;
}
