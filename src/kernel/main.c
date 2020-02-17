#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__linux__)
#error "You are not using a cross-compiler"
#endif

#include "../util/vga.h"
#include "../util/string.h"

int test_bss;
int64_t test_data = 42;

void kernel_main(void) {
	char nb[17];
	nb[16] = '\0';
	terminal_initialize();
	terminal_writestring("Kernel 64 bits lance\n");
	int64_to_str_hexa(nb, test_data);
	terminal_writestring(nb);
	terminal_writestring("\n");
}
