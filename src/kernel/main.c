#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__linux__)
#error "You are not using a cross-compiler"
#endif

#include <util/vga.h>
#include <util/string.h>

#include <kernel/memory/kmem.h>
#include <kernel/idt.h>
#include <kernel/gdt.h>
#include <kernel/tty.h>
#include <kernel/sys.h>
#include <kernel/proc.h>
#include <kernel/kutil.h>

#include <kernel/tests.h>

void kernel_init(uint32_t boot_info) {
    gdt_init();
    kmem_init_paging();
    tty_init(ttym_def);
    kmem_init_alloc(boot_info);
    tss_init();
    idt_init();
   
	vga_init((uint16_t*)(low_addr + VGA_BUFFER));
}

__attribute__ ((noreturn))
void kernel_main(uint32_t boot_info) {
    kernel_init(boot_info);

    klog(Log_info, "statut", "64 bits kernel launched.");
    klogf(Log_info, "mem", "%lld pages disponibles",
            (long long int)kmem_nb_page_free());

    tty_afficher_buffer_all();

    tty_new_prompt();

    proc_start();
}
