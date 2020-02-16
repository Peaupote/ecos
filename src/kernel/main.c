#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#if defined(__linux__)
#error "You are not using a cross-compiler"
#endif

#include "../util/vga.h"

int test_bss;
int test_data = 42;

void kernel_main(void) {
    terminal_initialize();
    printf("64 bits kernel launched.");
}
