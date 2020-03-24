#include <util/vga.h>
#include <libc/stdio.h>

int putchar(int p) {
    unsigned char c = (unsigned char)p;
    vga_putchar(c);
    return (int)c;
}
