#include <libc/string.h>

char *index(const char *src, int c) {
	return strchr(src, c);
}
