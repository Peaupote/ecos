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
#include "gdt.h"
#include "tty.h"
#include "sys.h"
#include "proc.h"

#include "tests.h"

void kernel_init(void) {
    kmem_init();
    terminal_init((uint16_t*)(low_addr + VGA_BUFFER));
    tty_init();
	gdt_init();
	tss_init();
    idt_init();
}

void kernel_main(void) {
    kernel_init();

    tty_writestring("64 bits kernel launched.\n");
    tty_afficher_buffer_all();

    tty_new_prompt();

    init();
    fork();
    fork();
    while (true) halt();
}
