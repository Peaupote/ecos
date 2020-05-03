#include <libc/unistd.h>

unsigned int sleep(unsigned int sec) {
	if (usleep(sec * 1000000L) == -1)
		return sec;
	return 0;
}
