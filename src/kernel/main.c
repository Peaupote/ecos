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
#include "keyboard.h"

#include "tests.h"

char num_str[17];
char key_str[4] = "' '";
extern uint64_t test_count;

void kernel_main(void) {
	char key_char;

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
		int64_to_str_hexa(num_str, keyboard_input_keycode);
		terminal_writestring(num_str);
		terminal_cursor_at(2, 50);
		key_char = keyboard_char_map[keyboard_input_keycode & 0x7f];
		if(key_char){
			key_str[1] = key_char;
			terminal_writestring(key_str);
		} else
			terminal_writestring("???");
		asm volatile("hlt" : : : "memory");
	}
}
