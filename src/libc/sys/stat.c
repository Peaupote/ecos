#include <libc/sys/stat.h>
#include <libc/unistd.h>
#include <libc/fcntl.h>

int lstat(const char *fname, struct stat *st) {
    int fd = open(fname, O_NOFOLLOW|O_RDONLY);
    if (fd < 0) return -1;

    int rc = fstat(fd, st);

    close(fd);
    return rc;
}

int stat(const char *fname, struct stat *st) {
    int fd = open(fname, O_RDONLY);
    if (fd < 0) return -1;

    int rc = fstat(fd, st);

    close(fd);
    return rc;
}
