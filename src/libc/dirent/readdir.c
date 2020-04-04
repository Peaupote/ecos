#include <libc/dirent.h>
#include <libc/sys.h>
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

    dir       = dirp->dir_entry;
    dirp->off += dir->d_rec_len; // TODO problem here
    dirp->pos += dir->d_rec_len;

    if (dirp->off > DIRP_BUF_SIZE) {
        lseek(dirp->fd, dirp->pos);
        if (read(dirp->fd, dirp->buf, DIRP_BUF_SIZE) < 0) goto error;
        dirp->dir_entry = (struct dirent*)dirp->buf;
    } else {
        dirp->dir_entry = (struct dirent*)((char*)dir + dirp->dir_entry->d_rec_len);
    }

    return dir;

error:
    return 0;
}
