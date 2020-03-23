#include <util/vga.h>
#include <libc/stdio.h>

int putchar(int p) {
    unsigned char c = (unsigned char)p;

    // problem here, libc is not supposed to be in the kernel
    #ifdef TEST
    vga_putchar(c);
    #endif
    return (int)c;
}
