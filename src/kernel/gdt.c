#include <kernel/gdt.h>

#include <stddef.h>

#include <util/gdt.h>
#include <kernel/memory/kmem.h>

struct TSS tss = {0};

void tss_init() {
    tss.rsp[0] = kernel_stack_top;
    asm volatile ("ltr %%ax"
            :: "rax"(SEG_SEL(offsetof(struct GDT, tss_low), 0))
             : "memory");
}

void gdt_init() {
    //ring0_code, ring0_data déjà initialisés

    gd_table.ring1_code = GDT_ENTRY_64(0, 0xfffff,
            GDT_ACC_P|GDT_ACC_Pr(1)|GDT_ACC_S|GDT_ACC_R|GDT_ACC_E,
            GDT_FLAG_G|GDT_FLAG_L);
    gd_table.ring1_data = GDT_ENTRY_64(0, 0xfffff,
            GDT_ACC_P|GDT_ACC_Pr(1)|GDT_ACC_S|GDT_ACC_R,
            GDT_FLAG_G|GDT_FLAG_S);

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
