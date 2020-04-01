#include <libc/stdio.h>
#include <libc/unistd.h>

int putchar(int p) {
	unsigned char c = (unsigned char)p;
	write(1, &c, 1);
	return c;
}
