#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main() {
    pid_t pid;
    int fds[2];
    int rc = pipe(fds);
    if (rc < 0) {
        perror("pipe");
        exit(1);
    }

    if (fork() == 0) {
        char buf[1024];
        pid = getpid();
        close(fds[1]);

        while ((rc = read(fds[0], buf, 1024)) > 0) {
            buf[rc] = 0;
            printf("[pid %d] %s", pid, buf);
        }

        if (rc < 0) {
            perror("write");
            exit(1);
        }

        close(fds[0]);
        exit(0);
    }

    char *str = "Hello !\n";
    int len = strlen(str);
    pid = getpid();
    close(fds[0]);

    while((rc = write(fds[1], str, len)) > 0) {
        sleep(2);
        printf("[pid %d] write \"%s\"", pid, str);
    }

    close(fds[1]);
    return 0;
}
