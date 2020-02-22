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

char num_str[17];
extern uint64_t test_count;
extern uint8_t test_data_in;

void kernel_main(void) {
    idt_init();
    kmem_init();
    terminal_initialize();
    terminal_writestring("64 bits kernel launched.\n");

	asm volatile("int $0x80" : : : "memory");
    terminal_writestring("Done.\n");
	
	num_str[16] = '\0';

    while(1) {
		terminal_cursor_at(0, 50);
		int64_to_str_hexa(num_str, test_count++);
		terminal_writestring(num_str);
		terminal_cursor_at(1, 50);
		int64_to_str_hexa(num_str, test_data_in);
		terminal_writestring(num_str);
//		asm volatile("hlt" : : : "memory");
	}
}
