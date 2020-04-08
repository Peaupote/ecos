#include <libc/stdlib.h>
#include <libc/string.h>
#include <libc/errno.h>
#include <libc/stdio.h>
#include <libc/assert.h>

static int nenv;

void _env_init(char **env_ptr) {
    char **ptr;

    memset(_env, 0, NENV);
    nenv = 0;

    for (ptr = env_ptr, nenv = 0; nenv + 1 < NENV && *ptr; ++ptr, ++nenv) {
        _env[nenv] = malloc(strlen(*ptr) + 1);
        strcpy(_env[nenv], *ptr);
    }

    _env[nenv] = 0;
}

void _env_cleanup() {
    for (char **ptr = _env; *ptr; ++ptr) free(*ptr);
}

char *getenv(const char *name) {
    for (char **ptr = _env; *ptr; ++ptr) {
        int len = 0; char *v = *ptr, *nptr = (char*)name;
        while (*v != '=' && (*nptr++ == *v++)) ++len;

        if (*v == '=' && !*nptr) {
            return ++v;
        }
    }

    return 0;
}

int setenv(const char *name, const char *value, int overwrite) {
    if (!name || !*name || index(name, '=')) {
        errno = EINVAL;
        return -1;
    }

    char **ptr;
    for (ptr = _env; *ptr; ++ptr) {
        int len = 0; char *v = *ptr, *nptr = (char*)name;
        while (*v != '=' && (*nptr++ == *v++)) ++len;

        if (*v == '=' && !*nptr) {
            if (overwrite) {
                int len = strlen(name) + strlen(value) + 2;
                char *s = calloc(len, 1);
                if (!s) goto err_enomem;

                sprintf(s, "%s=%s", name, value);
                free(*ptr);
                *ptr = s;
            }

            return 0;
        }
    }


    if (nenv + 1 == NENV) goto err_enomem;

    int len = strlen(name) + strlen(value) + 2;
    char *s = calloc(len, 1);
    if (!s) goto err_enomem;
    _env[nenv++] = s;
    _env[nenv] = 0;

    sprintf(s, "%s=%s", name, value);

err_enomem:
    errno = ENOMEM;
    return -1;
}
