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

int test_bss;

void kernel_main(void) {
	char nb_str[17];
	kmem_init();

    terminal_initialize();
    terminal_writestring("64 bits kernel launched.\n");
	nb_str[16] = '\0';
	terminal_writestring("addresse: virtuelle=");
	int64_to_str_hexa(nb_str, (uint_ptr)&test_bss);
	terminal_writestring(nb_str);
	terminal_writestring(" physique=");
	int64_to_str_hexa(nb_str, paging_phy_addr((uint_ptr)&test_bss));
	terminal_writestring(nb_str);
}
