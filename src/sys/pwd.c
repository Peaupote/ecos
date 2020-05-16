#include <unistd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static inline bool stat_eq(struct stat* a, struct stat* b) {
    return a->st_ino == b->st_ino && a->st_dev == b->st_dev;
}

int main(int argc __attribute__((unused)),
         char *argv[] __attribute__((unused))) {
    int pos = 1023;
    struct stat sts[2] = {0};
    struct stat *st = sts,
                *p  = sts + 1;
    char pwd[1024] = { 0 };

    stat(".", st);

    while (chdir(".."), stat(".", p), !stat_eq(p, st)) {
        DIR *dirp;

        // open current dir and look for previous dir name in this one
        dirp = opendir(".");
        if (!dirp) {
            perror("dirp");
            exit(1);
        }

        struct dirent *dir;
        struct stat c;
        do {
            if (! (dir = readdir(dirp)) ) {
                fprintf(stderr, "something unexpected just happend\n");
                closedir(dirp);
                return 1;
            }
        } while (p->st_dev == st->st_dev
                    ? dir->d_ino != st->st_ino
                    : (stat(dir->d_name, &c), !stat_eq(&c, st)) );

        // previous dir founded

        pos -= dir->d_name_len + 1;
        if (pos <= 0) {
            fprintf(stderr, "name to long : %s\n", pwd);
            exit(1);
        }

        pwd[pos + dir->d_name_len] = '/';
        memcpy(pwd + pos, dir->d_name, dir->d_name_len);

        closedir(dirp);

        // swap(p, st)
        struct stat* tmp = p;
        p  = st;
        st = tmp;
    }

    printf("/%s\n", pwd + pos);
    return 0;
}
