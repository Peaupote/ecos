#include <libc/dirent.h>
#include <libc/sys/stat.h>
#include <libc/fcntl.h>
#include <libc/stdlib.h>
#include <libc/unistd.h>
#include <libc/stdio.h>
#include <libc/string.h>

struct dirent *readdir(struct dirp *dirp) {
    if (!dirp) return 0;

    // size might change between readdirs
    if (dirp->pos == dirp->size) return 0;

    struct dirent *dir;

    dir        = dirp->dir_entry;
    dirp->off += dir->d_rec_len; // TODO problem here
    dirp->pos += dir->d_rec_len;

    if (dirp->off > dirp->bsz) {
		int rc = read(dirp->fd, dirp->buf, DIRP_BUF_SIZE);
		if (rc < 0) goto error;
		if (rc == 0) return 0;
		dirp->off = 0;
        dir = dirp->dir_entry = (struct dirent*)dirp->buf;
		if (rc < dir->d_rec_len) goto error; //nom trop long
    } else
        dir = dirp->dir_entry = (struct dirent*)(dirp->buf + dirp->off);

    return dir;

error:
    return NULL;
}
