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
#include "tty.h"

#include "tests.h"

void kernel_main(void) {
    terminal_init();
    tty_init();
    kmem_init();
    idt_init();

    tty_writestring("64 bits kernel launched.");
    tty_afficher_buffer_all();

    tty_new_prompt();

    // -- PIT init --
    outb(PIT_CONF_PORT, 0b00110000);
    outb(PIT_DATA_PORT0, 0);
    outb(PIT_DATA_PORT0, 0);

    while(true) halt();
}
