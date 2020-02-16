#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__linux__)
#error "You are not using a cross-compiler"
#endif

#include "../util/vga.h"

int test_bss;
int test_data = 42;

void kernel_main(void) {
	terminal_initialize();
	terminal_writestring("Kernel 64 bits lance");
}
