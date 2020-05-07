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

    for (ptr = env_ptr, nenv = 0; nenv + 1 < NENV && *ptr; ++ptr, ++nenv)
        _env[nenv] = strdup(*ptr);

    _env[nenv] = NULL;
}

char *getenv(const char *name) {
    for (char **ptr = _env; *ptr; ++ptr) {
		char *v = *ptr;
		const char* nptr = name;
        while (*v != '=' && (*nptr == *v)){
			++nptr;
			++v;
		}

        if (*v == '=' && !*nptr)
            return ++v;
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
        char *v = *ptr;
		const char *nptr = name;
        while (*v != '=' && *nptr == *v) {
			++nptr;
			++v;
		}

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
	return 0;

err_enomem:
    errno = ENOMEM;
    return -1;
}
