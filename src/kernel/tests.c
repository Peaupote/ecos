#include "tests.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../util/vga.h"
#include "../util/string.h"

#include "kmem.h"
#include "keyboard.h"
#include "idt.h"

char nb_str[17];

int test_bss;
void test_kmem() {
	phy_addr new_page;
	uint_ptr v_addr = 0x42424242000;

	nb_str[16] = '\0';
	int64_to_str_hexa(nb_str, end_kernel);
	terminal_writestring("end_kernel=");
	terminal_writestring(nb_str);
	terminal_writestring("\naddresse: virtuelle=");
	int64_to_str_hexa(nb_str, (uint_ptr)&test_bss);
	terminal_writestring(nb_str);
	terminal_writestring(" physique=");
	int64_to_str_hexa(nb_str, paging_phy_addr((uint_ptr)&test_bss));
	terminal_writestring(nb_str);

	new_page = kmem_alloc_page();
	terminal_writestring("\npa=");
	int64_to_str_hexa(nb_str, new_page);
	terminal_writestring(nb_str);
	terminal_writestring(" rt=");
	int64_to_str_hexa(nb_str, paging_map_to(v_addr, new_page));
	terminal_writestring(nb_str);
	*((unsigned char*)v_addr) = 42;
}

void test_idt() {
	char key_char;
	char key_str[4] = "' '";
 	uint64_t test_count = 0;
	nb_str[16] = '\0';

	asm volatile("int $0x80" : : : "memory");
    terminal_writestring("Done.\n");

    while(1) {
		terminal_cursor_at(0, 50);
		int64_to_str_hexa(nb_str, test_count++);
		terminal_writestring(nb_str);
		terminal_cursor_at(1, 50);
		int64_to_str_hexa(nb_str, keyboard_input_keycode);
		terminal_writestring(nb_str);
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
