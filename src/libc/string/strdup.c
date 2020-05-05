#include <libc/string.h>
#include <libc/stdlib.h>

char *strdup(const char *s) {
	size_t len = strlen(s);
	char*   rt = malloc((len + 1) * sizeof(char));
	memcpy(rt, s, len + 1);
	return rt;
}
