#include <kernel/tests.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <util/vga.h>
#include <util/string.h>

#include <kernel/memory/kmem.h>
#include <kernel/keyboard.h>
#include <kernel/idt.h>
#include <kernel/gdt.h>
#include <kernel/kutil.h>

char nb_str[17];

int test_bss;
void test_kmem() {
    phy_addr new_page;
    uint_ptr v_addr = 0x42424242000;

    nb_str[16] = '\0';
    int64_to_str_hexa(nb_str, end_kernel);
    vga_writestring("end_kernel=");
    vga_writestring(nb_str);
    vga_writestring("\naddresse: virtuelle=");
    int64_to_str_hexa(nb_str, (uint_ptr)&test_bss);
    vga_writestring(nb_str);
    vga_writestring(" physique=");
    int64_to_str_hexa(nb_str, paging_phy_addr((uint_ptr)&test_bss));
    vga_writestring(nb_str);

    new_page = kmem_alloc_page();
    vga_writestring("\npa=");
    int64_to_str_hexa(nb_str, new_page);
    vga_writestring(nb_str);
    vga_writestring(" rt=");
    int64_to_str_hexa(nb_str, 
			paging_map_to(v_addr, new_page, PAGING_FLAG_W, PAGING_FLAG_W));
    vga_writestring(nb_str);
    *((unsigned char*)v_addr) = 42;
}

void test_idt() {
    uint64_t test_count = 0;
    nb_str[16] = '\0';

    asm volatile("int $0x80" : : : "memory");
    vga_writestring("Done.\n");

    while(1) {
        vga_cursor_at(0, 50);
        int64_to_str_hexa(nb_str, test_count++);
        vga_writestring(nb_str);
        asm volatile("hlt" : : : "memory");
    }
}

uint8_t test_struct_layout() {
    return
        sizeof(struct      GDT) == 0x38
     && sizeof(struct GDT_desc) == 0xa
     && sizeof(struct      TSS) == 0x68;
}

void test_print_statut(void) {
    uint64_t rsp;
    uint16_t ss;
    asm("movq %%rsp, %0" : "=r"(rsp));
    asm("movw %%ss, %0" : "=r"(ss));
    klogf(Log_info, "test", "rsp=%p ss=%x", rsp, (unsigned int)ss);
}

void test_kheap(void) {
    void* addr[3];
    for (uint8_t i = 0; i < 3; ++i) {
        addr[i] = kalloc_page();
        kprintf("kalloc %p\n", addr[i]);
    }
    for (uint8_t i = 0; i < 3; ++i) {
        kfree_page(addr[i]);
        kprintf("kfree  %p\n", addr[i]);
    }
}
