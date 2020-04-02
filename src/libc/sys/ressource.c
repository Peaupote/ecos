#include <libc/sys.h>

int setpriority(int prio) {
	return _setpriority(19 - prio);
}

int getpriority() {
	return 19 - _getpriority();
}
