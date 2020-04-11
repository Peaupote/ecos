#include <libc/dirent.h>
#include <libc/sys/stat.h>
#include <libc/fcntl.h>
#include <libc/stdlib.h>
#include <libc/unistd.h>
#include <libc/stdio.h>
#include <libc/string.h>

struct dirent *readdir(struct dirp *dirp) {
    if (!dirp) return NULL;

    struct dirent *dir;

    if (dirp->off >= dirp->bsz) {
		int rc = read(dirp->fd, dirp->buf, DIRP_BUF_SIZE);
		if (rc  < 0) goto error;
		if (rc == 0) return 0;
        dir = (struct dirent*)dirp->buf;
		if (rc < dir->d_rec_len) goto error; //nom trop long
		dirp->off = 0;
		dirp->bsz = rc;
    } else
		dir = (struct dirent*)(dirp->buf + dirp->off);
  
#ifdef __is_debug
	if (!dir->d_rec_len) {
		printf("erreur: dir->d_rec_len = 0\n");
		return NULL;
	}
#endif

	dirp->off += dir->d_rec_len;
    dirp->pos += dir->d_rec_len;

	return dir;

error:
    return NULL;
}
