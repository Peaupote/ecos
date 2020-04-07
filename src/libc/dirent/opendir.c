#include <libc/dirent.h>
#include <libc/sys/stat.h>
#include <libc/fcntl.h>
#include <libc/stdlib.h>
#include <libc/unistd.h>
#include <libc/stdio.h>

struct dirp *opendir(const char *fname) {
    int fd = open(fname, READ);
    struct stat st;
    struct dirp *dirp = 0;

    fstat(fd, &st);
    if (!(st.st_mode&TYPE_DIR)) goto error;

    dirp = malloc(sizeof(struct dirp));
    if (!dirp) goto error;

    if (read(fd, dirp->buf, DIRP_BUF_SIZE) < 0) goto error;

    dirp->fd        = fd;
    dirp->size      = st.st_size;
    dirp->pos       = 0;
    dirp->off       = 0;
    dirp->dir_entry = (struct dirent*)dirp->buf;
    return dirp;

error:
    close(fd);
    free(dirp);
    return 0;
}

int closedir(struct dirp *dir) {
    if (!dir) return -1;
    int fd = dir->fd;
    free(dir);
    return close(fd);
}
