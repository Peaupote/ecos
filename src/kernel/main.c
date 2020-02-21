#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#if defined(__linux__)
#error "You are not using a cross-compiler"
#endif

#include "../util/vga.h"
#include "../util/string.h"

#include "kmem.h"
#include "idt.h"

#include "tests.h"

void kernel_main(void) {
    idt_init();
    kmem_init();
    terminal_initialize();
    terminal_writestring("64 bits kernel launched.\n");

    asm volatile("int $0x80" : : : "memory");
    terminal_writestring("Done.\n");

    while(1);
}
