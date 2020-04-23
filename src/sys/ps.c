#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

int is_not_pid(const char *s) {
    while (*s >= '0' && *s <= '9') ++s;
    return *s;
}

int main () {
    struct dirp *dirp = opendir("/proc");
    struct dirent *dir;
    char buf[256];
    int pid, count = 0;

    printf("PID\tSTATE\tCMD\n");
    while ((dir = readdir(dirp))) {
        memcpy(buf, dir->d_name, dir->d_name_len);
        buf[dir->d_name_len] = 0;

        if (is_not_pid(buf)) continue;

        pid = atoi(buf);
        sprintf(buf, "/proc/%d/stat", pid);
        int fd = open(buf, O_RDONLY);
        if (fd < 0) {
            perror("ps: open");
            closedir(dirp);
            exit(1);
        }

        if (read(fd, buf, 1024) < 0) {
            perror("ps: read");
            close(fd);
            closedir(dirp);
            exit(1);
        }

        int p; char cmd[256]; char st;
        sscanf(buf, "%d %s %c", &p, cmd, &st);

        printf("%d\t%c\t\t%s\n", pid, st, cmd);

        close(fd);

        count++;
    }

    printf("total %d\n", count);

    closedir(dirp);
    return 0;
}
