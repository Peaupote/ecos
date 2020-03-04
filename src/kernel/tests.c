#include "tests.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../util/vga.h"
#include "../util/string.h"

#include "kmem.h"
#include "keyboard.h"
#include "idt.h"
#include "gdt.h"

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
 	uint64_t test_count = 0;
	nb_str[16] = '\0';

	asm volatile("int $0x80" : : : "memory");
    terminal_writestring("Done.\n");

    while(1) {
		terminal_cursor_at(0, 50);
		int64_to_str_hexa(nb_str, test_count++);
		terminal_writestring(nb_str);
		asm volatile("hlt" : : : "memory");
	}
}

uint8_t test_struct_layout() {
	return
		sizeof(struct      GDT) == 0x38
	 && sizeof(struct GDT_desc) == 0xa
	 && sizeof(struct      TSS) == 0x68;
}
