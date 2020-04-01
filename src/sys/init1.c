#include <libc/sys.h>
#include <libc/stdio.h>
#include <libc/string.h>

void child() {
#define SZ 2
    pid_t pid = getpid();
    int i, rc;
    char buf[SZ];
    while (1) {
        sleep(1);
        rc = read(0, buf, SZ);
        if (rc < 0) {
            printf("error read stdin");
            exit(1);
        } else if (rc > 0) {
            printf("[pid %d] read %d chars : ", pid, rc);
            for (i = 0; i < rc; i++) printf("%c", buf[i]);
            printf("\n");
        } else {
            printf("[pid %d] nothing on stdin\n", pid);
        }
    }
}

int main() {
    r1_prires(3);
    //On est désormais en ring 3

    printf("Hello world !\n");
    char *str = "aabbccdd";
    size_t len = strlen(str), n = 32;

    pid_t ppid = getpid(), pid;
    if ((pid = fork()) == 0) child();
    else {
        char fname[256];
        sprintf(fname, "/proc/%d/fd/0", pid);
        int fd = open(fname, WRITE);
        if (fd < 0) exit(1);

        while(1) {
            printf("[pid %d] write %s %d times on %s\n", ppid, str, n, fname);
            for (size_t k = 0; k < n; k++)
                write(fd, str, len);
            sleep(4);
        }

        close(fd);
    }

    return 0;
}
