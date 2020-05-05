#include <libc/stdlib.h>
#include <libc/ctype.h>

#include <stdbool.h>

int atoi(const char *s) {
	while (isspace(*s)) ++s;
	bool neg = false;
	if (*s == '+')
		++s;
	else if (*s == '-') {
		++s;
		neg = true;
	}
    int n = 0;
    for(; '0' <= *s && *s <= '9'; s++)
        n = n * 10 + (*s - '0');

    return neg ? -n : n;
}
