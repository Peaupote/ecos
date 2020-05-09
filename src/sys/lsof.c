#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <assert.h>

int is_not_pid(const char *s) {
    while (*s >= '0' && *s <= '9') ++s;
    return *s;
}

void list_files(pid_t pid) {
    char buf[256], buf2[256];

    // get process command
    char cmd[256];
    sprintf(buf, "/proc/%d/cmd", pid);
    int fd = open(buf, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "lsof: %d: invalid pid\n", pid);
        exit(1);
    }

    int rc = read(fd, cmd, 256);
    cmd[rc] = 0;
    close(fd);

    sprintf(buf, "/proc/%d/fd", pid);
    DIR *dirp = opendir(buf);
    struct dirent *dir;
    struct stat statbuf;

    while ((dir = readdir(dirp))) {
        // test not . neither ..
        if (is_not_pid(dir->d_name)) continue;

        sprintf(buf, "/proc/%d/fd/%s", pid, dir->d_name);
        stat(buf, &statbuf);
        printf("%15s ", cmd);
        switch (statbuf.st_mode&0xf000) {
        case TYPE_FIFO: printf("FIFO "); break;
        case TYPE_CHAR: printf("CHAR "); break;
        case TYPE_DIR:  printf(" DIR "); break;
        case TYPE_BLK:  printf(" BLK "); break;
        case TYPE_REG:  printf(" REG "); break;
        case TYPE_SYM:  printf(" SYM "); break;
        case TYPE_SOCK: printf("SOCK "); break;
        default:        printf("  ?? "); break;
        }

        printf("%10d ", statbuf.st_size);

        rc = readlink(buf, buf2, 256);
        buf2[rc] = 0;
        printf("%s\n", buf2);
    }

    closedir(dirp);
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (is_not_pid(argv[i])) {
            fprintf(stderr, "lsof: %s: not a pid\n", argv[i]);
            return 1;
        }

        list_files(atoi(argv[i]));
    }

    if (argc == 1) {
        DIR *dirp = opendir("/proc");
        struct dirent *dir;

        while ((dir = readdir(dirp))) {
            if (is_not_pid(dir->d_name)) continue;
            list_files(atoi(dir->d_name));
        }

        closedir(dirp);
    }

    return 0;
}
