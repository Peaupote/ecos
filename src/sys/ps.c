#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys.h>
#include <string.h>
#include <stdlib.h>

int main () {
    struct dirp *dirp = opendir("/proc");
    struct dirent *dir;
    char buf[256];
    int pid, count = 0;

    printf("PID\tSTATE\tCMD\n");
    while ((dir = readdir(dirp))) {
        memcpy(buf, dir->d_name, dir->d_name_len);
        buf[dir->d_name_len] = 0;

        if (!strcmp(buf, ".") || !strcmp(buf, "..")) continue;

        pid = atoi(buf);
        sprintf(buf, "/proc/%d/stat", pid);
        int fd = open(buf, READ);

        read(fd, buf, 1024);
        int p; char cmd[256]; char st;
        sscanf(buf, "%d %s %c", &p, cmd, &st);

        printf("%d\t%c\t%s\n", pid, st, cmd);

        close(fd);

        count++;
    }

    printf("total %d\n", count);

    closedir(dirp);
    return 0;
}
