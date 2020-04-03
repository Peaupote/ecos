#include <libc/string.h>

static char *save = 0;

char *strtok(char *src, const char *delim) {
	return strtok_r(src, delim, &save);
}
