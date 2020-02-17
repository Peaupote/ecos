#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#if defined(__linux__)
#error "You are not using a cross-compiler"
#endif

#include "../util/vga.h"
#include "int.h"

int test_bss;
int test_data = 42;

void kernel_main(void) {
    printf("64 bits kernel launched.\n");

    while(1);
}
