#include "../../util/vga.h"
#include <stdio.h>

int putchar(int p) {
    unsigned char c = (unsigned char)p;

    // problem here, libc is not supposed to be in the kernel
    terminal_putchar(c);
    return (int)c;
}
