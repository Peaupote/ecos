#include <libc/unistd.h>
#include <libc/stdlib.h>
#include <libc/stdio.h>
#include <libc/string.h>
#include <libc/dirent.h>
#include <libc/errno.h>

static char *lookup_path(const char *fname, char *rt) {
    const char *path = getenv("PATH");
    char *cpy = calloc(strlen(path) + 1, 1);
    strcpy(cpy, path);

    struct dirp *dirp;
    struct dirent *dir;
    int name_len = strlen(fname);

    char *p, *svptr;
    for (p = strtok_r(cpy, ":", &svptr); p; p = strtok_r(0, ":", &svptr)) {
        dirp = opendir(p);
        if (!dirp) {
            continue;
        }

        while ((dir = readdir(dirp))) {
            if (name_len == dir->d_name_len &&
                strncmp(dir->d_name, fname, dir->d_name_len)) {
                sprintf(rt, "%s/%s", p, fname);
                return rt;
            }
        }

        closedir(dirp);
    }

    return 0;
}

int execv(const char *fname, const char **argv) {
    return execve(fname, argv, (const char**)_env);
}

// dont lookup when start with ./
int execvp(const char *name, const char **argv) {
    char fname[256] = { 0 };
    if(!lookup_path(name, fname)) {
        errno = ENOENT;
        return -1;
    }
    return execv(fname, argv);
}

int execvpe(const char* name, const char** argv, const char **env) {
    char fname[256] = { 0 };
    if(!lookup_path(name, fname)) {
        errno = ENOENT;
        return -1;
    }
    return execve(fname, argv, env);
}
