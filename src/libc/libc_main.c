#include "stdlib/stdlib.h"
#include "signal/signal.h"

void _libc_init() {
	_signal_init();
	_malloc_init();
}
