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

    char str[]   = "__..__..__..__..\n";
    size_t idx_b = tty_buffer_next_idx();
    size_t shift = tty_writestring("nb proc");
    int64_to_str_hexa(str, state.st_waiting_ps);
    shift += tty_writestring(str);
    shift += tty_writestring("\n");
    if (shift) tty_afficher_buffer_all();
    else tty_afficher_buffer_range(idx_b, tty_buffer_next_idx());

    while (true) halt();
}
