#include <libc/sys.h>

int setpriority(int prio) {
	return _setpriority(19 - prio);
}

int getpriority() {
	return 19 - _getpriority();
}

int nice(int inc) {
	int prio = getpriority();
	prio += inc;
	return setpriority(prio) ? - 1 : prio;
}
