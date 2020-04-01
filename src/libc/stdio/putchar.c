#include <util/vga.h>
#include <libc/stdio.h>

int putchar(int p) {
	//TODO: if libc
#ifdef __is_kernel
    unsigned char c = (unsigned char)p;
    vga_putchar(c);
    return (int)c;
#endif
	return p;
}
