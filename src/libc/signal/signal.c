#include <libc/signal.h>
#include <libc/sys.h>

#include <libc/stdio.h>

sighandler_t sig_handlers[SIG_COUNT];

extern void libc_sighandler(int signum);

void _signal_init() {
	_sigsethnd(&libc_sighandler);
}

sighandler_t signal(int signum, sighandler_t hnd) {
	if (signum < 1 || signum > SIG_COUNT)
		return SIG_ERR;
	int sigid = signum - 1;

	uint8_t hn = 0;
	if      (hnd == SIG_IGN) hn = 1;
	else if (hnd == SIG_DFL) hn = 2;

	uint8_t pr = _signal(sigid, hn);
	
	if (pr == (uint8_t)~0)
		return SIG_ERR;

	sighandler_t prh = sig_handlers[sigid];
	sig_handlers[sigid] = hnd;
	
	return pr ? (sighandler_t)(uintptr_t)(pr - 1) : prh;
}
