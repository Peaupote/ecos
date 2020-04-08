#include "stdlib/stdlib.h"
#include "signal/signal.h"

extern void _errno_init();

void _libc_init() {
    _errno_init();
    _signal_init();
    _malloc_init();
}
