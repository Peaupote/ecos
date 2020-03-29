#include <util/vga.h>
#include <libc/stdio.h>

int putchar(int p) {
	//TODO: if libc
#if defined(__is_libk)
    unsigned char c = (unsigned char)p;
    vga_putchar(c);
    return (int)c;
#endif
	return p;
}
