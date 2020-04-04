#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys.h>
#include <string.h>
#include <stdlib.h>

static int flag_inode = 0;
static int flag_long  = 0;

#define MODE_LEN 10
void sprint_mode(mode_t mode, char *buf) {
    switch (mode&0xf000) {
    case TYPE_DIR:  *buf++ = 'd'; break;
    case TYPE_CHAR: *buf++ = 'c'; break;
    case TYPE_FIFO: *buf++ = 'p'; break;
    case TYPE_BLK:  *buf++ = 'b'; break;
    case TYPE_SYM:  *buf++ = 'l'; break;
    case TYPE_SOCK: *buf++ = 's'; break;
    case TYPE_REG:  *buf++ = '-'; break;
    }

    char c[3] = { 'x', 'w', 'r' };
    for (size_t i = 0; i < 9; i++) {
        buf[9 - i - 1] = mode & (1 << i) ? c[i%3] : '-';
    }
}

int list_files(const char *fname) {
    struct dirp *dirp = opendir(fname);
    struct dirent *dir;
    struct stat st;
    char buf[256], buf2[1024], bufmode[MODE_LEN + 1] = { 0 };
    int rc = 0;

    while ((dir = readdir(dirp))) {
        memcpy(buf, dir->d_name, dir->d_name_len);
        buf[dir->d_name_len] = 0;

        sprintf(buf2, "%s/%s", fname, buf);
        rc = stat(buf2, &st);
        if (rc < 0) {
            printf("error on stat %s\n", buf2);
            break;
        }

        if (flag_inode) {
            printf("%d ", st.st_ino);
        }

        if (flag_long) {
            sprint_mode(st.st_mode, bufmode);
            printf("%s %d %d ", bufmode, st.st_size, st.st_nlink);
        }

        printf("%s\n", buf);
    }

    closedir(dirp);
    return rc;
}

void read_flags(const char *flags) {
    while (*++flags) {
        switch (*flags) {
        case 'i': flag_inode = 1; break;
        case 'l': flag_long  = 1; break;
        default:
            printf("unknown flag %c\n", *flags);
            exit(1);
        }
    }
}

int main (int argc, char *argv[]) {
    int i, rc = 0, count = 0;
    char *fnames[256], **fptr = fnames;
    char *cwd = "/home"; // TODO getcwd

    for (i = 1; i < argc; i++) {
        if (*argv[i] == '-') read_flags(argv[i]);
        else {
            *fptr++ = argv[i];
            count++;
        }
    }

    if (count == 0) {
        fnames[0] = cwd;
        count++;
    }

    for (i = 0; i < count; i++) {
        rc = list_files(fnames[i]);
        if (rc < 0) {
            printf("error on %s\n", fnames[i]);
            return rc;
        }
    }

    return 0;
}
