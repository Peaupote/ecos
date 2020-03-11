#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__linux__)
#error "You are not using a cross-compiler"
#endif

#include "../util/vga.h"
#include "../util/string.h"

#include "memory/kmem.h"
#include "idt.h"
#include "gdt.h"
#include "tty.h"
#include "sys.h"
#include "proc.h"
#include "kutil.h"

#include "tests.h"

void kernel_init(uint32_t boot_info) {
    gdt_init();
    kmem_init_paging();
    vga_init((uint16_t*)(low_addr + VGA_BUFFER));
    tty_init();
	kmem_init_alloc(boot_info);
    tss_init();
    idt_init();
}

void kernel_main(uint32_t boot_info) {
    kernel_init(boot_info);

    klog(Log_info, "statut", "64 bits kernel launched.");
    klogf(Log_info, "mem", "%d pages disponibles",
			(int)kmem_nb_page_free());

    tty_afficher_buffer_all();

    tty_new_prompt();

    init();

    while (true) halt();
}
