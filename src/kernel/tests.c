#include <kernel/tests.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <kernel/memory/kmem.h>
#include <kernel/keyboard.h>
#include <kernel/idt.h>
#include <kernel/gdt.h>
#include <kernel/kutil.h>

uint8_t test_struct_layout() {
    return
        sizeof(struct      GDT) == 0x48
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
