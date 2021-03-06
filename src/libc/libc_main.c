#include "stdlib/stdlib.h"
#include "signal/signal.h"

#include <libc/stdlib.h>
#include <libc/stdio.h>

extern void _exit(int status);
extern void _errno_init();
extern void _env_init();

void _libc_init(int argc __attribute__((unused)),
                char *argv[] __attribute__((unused)), char *penv[]) {
    _errno_init();
    _signal_init();
    _malloc_init();
    _env_init(penv);

    stdin  = fdopen(0, "r");
    stdout = fdopen(1, "w");
    stderr = fdopen(2, "w");
}

void _libc_exit(int code) {
    fclose(stdin);
    fclose(stdout);
    fclose(stderr);

    _exit(code);
}
