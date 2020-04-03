#include <libc/sys.h>
#include <libc/unistd.h>
#include <libc/fcntl.h>

int stat(const char *fname, struct stat *st) {
    int fd = open(fname, READ);
    if (fd < 0) return -1;

    int rc = fstat(fd, st);

    close(fd);
    return rc;
}
