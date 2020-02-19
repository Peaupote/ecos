#include "tests.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../util/vga.h"
#include "../util/string.h"

#include "kmem.h"

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
