#include <libc/unistd.h>
#include <libc/stdlib.h>
#include <libc/stdio.h>
#include <libc/string.h>
#include <libc/dirent.h>
#include <libc/errno.h>

static char *lookup_path(const char *fname) {
	if (fname[0]=='.' && fname[1]=='/') return NULL;

    const char *path = getenv("PATH");
	if (!path) return NULL;

    char *cpy = strdup(path);

    struct dirp *dirp;
    struct dirent *dir;
    int name_len = strlen(fname);

    char *p, *svptr;
    for (p = strtok_rnul(cpy, ":", &svptr); p;
				p = strtok_rnul(0, ":", &svptr)) {
        dirp = opendir(p);
        if (!dirp) {
            continue;
        }

        while ((dir = readdir(dirp))) {
            if (name_len == dir->d_name_len &&
                !strncmp(dir->d_name, fname, dir->d_name_len)) {
				char* rt = malloc(strlen(p) + 1 + name_len + 1);
                sprintf(rt, "%s/%s", p, fname);
				free(cpy);
                return rt;
            }
        }

        closedir(dirp);
    }

	free(cpy);

    return NULL;
}

int execv(const char *fname, const char **argv) {
    return execve(fname, argv, (const char**)_env);
}

int execvp(const char *name, const char **argv) {
	char *in_path = lookup_path(name);
    int rt = execv(in_path ? in_path : name, argv);
	free(in_path);
	return rt;
}

int execvpe(const char* name, const char** argv, const char **env) {
	char *in_path = lookup_path(name);
    int rt = execve(in_path ? in_path : name, argv, env);
	free(in_path);
	return rt;
}
