#include <unistd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc __attribute__((unused)),
         char *argv[] __attribute__((unused))) {
    int pos = 1023;
    struct stat st = { 0 }, p = { 0 };
    char pwd[1024] = { 0 };

    DIR *dirp;
    struct dirent *dir;

    stat(".", &st);

    while (1) {
        stat("..", &p);
        if (st.st_ino == p.st_ino && st.st_dev == p.st_dev) {
            printf("/%s\n", pwd + pos);
            return 0;
        }

        dirp = opendir("..");
        if (!dirp) {
            perror("dirp");
            exit(1);
        }

        while ((dir = readdir(dirp))) {
            if (dir->d_ino == st.st_ino) break;
        }

        if (!dir) {
            fprintf(stderr, "something unexpected just happend\n");
            exit(1);
        };

        pos -= dir->d_name_len + 1;
        if (pos <= 0) {
            fprintf(stderr, "name to long : %s\n", pwd);
            exit(1);
        }

        pwd[pos + dir->d_name_len] = '/';
        for (int i = 0; i < dir->d_name_len; ++i)
            pwd[pos + i] = dir->d_name[i];

        closedir(dirp);
        st = p;
        chdir("..");
    }
}
