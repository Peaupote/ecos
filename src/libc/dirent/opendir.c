#include <libc/dirent.h>
#include <libc/sys/stat.h>
#include <libc/fcntl.h>
#include <libc/stdlib.h>
#include <libc/unistd.h>
#include <libc/stdio.h>
#include <libc/errno.h>

struct dirp *opendir(const char *fname) {
    int serr, fd = open(fname, READ);
    struct stat st;
    struct dirp *dirp = 0;

    if (fd < 0) goto error;

    fstat(fd, &st);
    if (!(st.st_mode&TYPE_DIR)) goto error;

    dirp = malloc(sizeof(struct dirp));
    if (!dirp) goto error;

	int rc = read(fd, dirp->buf, DIRP_BUF_SIZE);
    if (rc <= 0) goto error;
    dirp->dir_entry = (struct dirent*)dirp->buf;
	if (rc < dirp->dir_entry->d_rec_len) goto error; //nom trop long

    dirp->fd        = fd;
    dirp->size      = st.st_size;
    dirp->pos       = 0;
    dirp->off       = 0;
	dirp->bsz       = rc;
    return dirp;

error:
    serr = errno;
    close(fd);
    free(dirp);
    errno = serr;
    return 0;
}

int closedir(struct dirp *dir) {
    if (!dir) return -1;
    int fd = dir->fd;
    free(dir);
    return close(fd);
}
