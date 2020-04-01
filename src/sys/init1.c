#include <libc/sys.h>
#include <libc/stdio.h>
#include <libc/string.h>

void child() {
#define SZ 1024
    pid_t pid = getpid();
    int rc;
    char buf[SZ];
    while (1) {
        rc = read(0, buf, SZ);
        if (rc < 0) {
            printf("error read stdin");
            exit(1);
        } else if (rc > 0) {
            buf[rc] = 0;
            printf("[pid %d] read %d chars : %s\n", pid, rc, buf);
        } else printf("NOT BLOCKING");
    }
}

int main() {
    r1_prires(3);
    //On est désormais en ring 3

    printf("Hello world !\n");

    char str[256] = { 0 };
    int len;

    /* while (1) { */
    /*     printf("Enter a string:\n"); */
    /*     len = read(0, str, 255); */
    /*     str[len] = 0; */
    /*     printf("'%s'\n", str); */
    /* } */

    // commente la boucle while ci dessus
    // les deux process partagent la meme entre std
    // quand on ecrit dans c'est une course a celui qui lit en premier

    pid_t ppid = getpid(), pid;
    if ((pid = fork()) == 0) child();
    else {
        printf("Enter a string:\n");
        len = read(0, str, 255);
        str[len] = 0;
        printf("[pid %d] read %s\n", ppid, str);

        char fname[256];
        sprintf(fname, "/proc/%d/fd/0", pid);
        int fd = open(fname, WRITE);
        if (fd < 0) exit(1);

        while(1) {
            printf("[pid %d] write %s on %s\n", ppid, str, fname);
            write(fd, str, len);
            sleep(2);
        }

        close(fd);
    }

    return 0;
}
