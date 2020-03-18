#include <kernel/gdt.h>

#include <stddef.h>

#include <util/gdt.h>
#include <kernel/memory/kmem.h>

struct TSS tss = {0};

extern uint8_t stack_top[];

uint8_t ist_stack_1[0x1000] __attribute__ ((aligned (0x10)));

void tss_init() {
    tss.rsp[0] = stack_top;
    tss.ist[0] = ist_stack_1 + 0x1000;
    asm volatile ("ltr %%ax"
            :: "rax"(offsetof(struct GDT, tss_low))
             : "memory");
}

void gdt_init() {
    //kernel_code, kernel_data déjà initialisés

    gd_table.ring3_code = GDT_ENTRY_64(0, 0xfffff,
            GDT_ACC_P|GDT_ACC_Pr(3)|GDT_ACC_S|GDT_ACC_R|GDT_ACC_E,
            GDT_FLAG_G|GDT_FLAG_L);
    gd_table.ring3_data = GDT_ENTRY_64(0, 0xfffff,
            GDT_ACC_P|GDT_ACC_Pr(3)|GDT_ACC_S|GDT_ACC_R,
            GDT_FLAG_G|GDT_FLAG_S);

    uint_ptr tss_addr = (uint_ptr)&tss;
    gd_table.tss_low = GDT_ENTRY_64(
            (uint32_t)tss_addr, sizeof(struct TSS),
            GDT_ACC_P|GDT_ACC_Pr(0)|GDT_ACC_A|GDT_ACC_E,
            GDT_FLAG_S);
    gd_table.tss_high = tss_addr >> 32;

    gdt_desc.limit = sizeof(struct GDT) - 1;
    gdt_desc.base  = &gd_table;
    asm volatile ("lgdt (%0)" :: "r"(&gdt_desc));
}
