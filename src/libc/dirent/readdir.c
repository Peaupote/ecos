#include <libc/dirent.h>
#include <libc/sys.h>
#include <libc/fcntl.h>
#include <libc/stdlib.h>
#include <libc/unistd.h>
#include <libc/stdio.h>

struct dirent *readdir(struct dirp *dirp) {
    if (!dirp) return 0;
    if (dirp->pos == dirp->size) return 0;

    struct dirent *dir;

    // problem if dir entry is cut in middle of buf
    if (dirp->off == DIRP_BUF_SIZE) {
        if(read(dirp->fd, dirp->buf, DIRP_BUF_SIZE) < 0) {
            // TODO
            exit(42);
        }

        dirp->dir_entry = (struct dirent*)dirp->buf;
    }

    dir = dirp->dir_entry;
    dirp->off += dirp->dir_entry->d_rec_len; // TODO problem here
    dirp->pos += dirp->dir_entry->d_rec_len;

    dirp->dir_entry = (struct dirent*)((char*)dirp->dir_entry
                                      + dirp->dir_entry->d_rec_len);

    return dir;
}
