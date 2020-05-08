#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__linux__)
#error "You are not using a cross-compiler"
#endif

#include <util/vga.h>

#include <kernel/memory/kmem.h>
#include <kernel/idt.h>
#include <kernel/gdt.h>
#include <kernel/tty.h>
#include <kernel/sys.h>
#include <kernel/proc.h>
#include <kernel/kutil.h>
#include <kernel/display.h>
#include <util/multiboot.h>

#include <kernel/tests.h>

static void kernel_init(uint32_t boot_info) {
    gdt_init();
    kmem_init_paging();

    kmem_bind_dynamic_range(0,
            boot_info, boot_info + sizeof(multiboot_info_t));
    multiboot_info_t* mbi = (multiboot_info_t*)
            kmem_dynamic_slot_at(0, boot_info);

    kmem_init_alloc(mbi);

    if (!display_init(mbi)) kpanic("Display");
    tty_init(ttym_def);
    klog_level = Log_warn;

    vfs_init();
    proc_init();

    tss_init();
    idt_init();
    
	kpanic_is_early = false;
}

__attribute__ ((noreturn))
void kernel_main(uint32_t boot_info) {
    kernel_init(boot_info);

    klog(Log_info, "statut", "64 bits kernel launched.");
    klogf(Log_info, "mem", "%lld pages disponibles",
            (long long int)kmem_nb_page_free());

    tty_afficher_buffer_all();

    proc_start();
}
