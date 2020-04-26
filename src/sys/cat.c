#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    char buf[1024];
    int fd = STDIN_FILENO, i = 0;
    ssize_t rc;

    if (argc == 1) goto start;

    for (i = 1; i < argc; i++) {
        fd = open(argv[i], O_RDONLY);
        if (fd < 0) {
            sprintf(buf, "cat: %s", argv[i]);
            perror(buf);
            continue;
        }

    start:
        while ((rc = read(fd, buf, 1024)) > 0)
            write(STDOUT_FILENO, buf, rc);

        if (rc < 0) {
			if (errno == EINTR)
				goto start;
            sprintf(buf, "cat: %s", i ? argv[i] : "stdin");
            perror(buf);
            exit(1);
        }

        close(fd);
    }

    return 0;
}
