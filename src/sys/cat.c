#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    char buf[1024];
    int fd = 0, i = 1, rc;

    if (argc == 1) goto start;

    for (; i < argc; i++) {
        fd = open(argv[i], READ);
        if (fd < 0) {
            printf("file %s not found\n", argv[i]);
            continue;
        }

    start:
        while ((rc = read(fd, buf, 1024)) > 0)
            write(1, buf, rc);

        if (rc < 0) {
            printf("error\n");
            exit(1);
        }
    }

    return 0;
}
