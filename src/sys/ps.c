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

    printf("PID\n");
    while ((dir = readdir(dirp))) {
        memcpy(buf, dir->d_name, dir->d_name_len);
        buf[dir->d_name_len] = 0;

        if (!strcmp(buf, ".") || !strcmp(buf, "..")) continue;

        pid = atoi(buf);
        printf("%d\n", pid);
        count++;
    }

    printf("total %d\n", count);

    closedir(dirp);
    return 0;
}
