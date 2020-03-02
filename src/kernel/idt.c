#include "idt.h"

#include <stddef.h>
#include <stdint.h>

#include "int.h"
#include "sys.h"
#include "../util/vga.h"
#include "../util/string.h"
#include "kmem.h"
#include "keyboard.h"
#include "tty.h"

extern void irq_sys(void);
extern void irq_keyboard(void);
extern void irq_pit(void);
extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();
extern void irq16();
extern void irq17();
extern void irq18();
extern void irq19();
extern void irq20();
extern void irq21();
extern void irq22();
extern void irq23();
extern void irq24();
extern void irq25();
extern void irq26();
extern void irq27();
extern void irq28();
extern void irq29();
extern void irq30();
extern void irq31();

const idt_handler int_handlers[NEXCEPTION_VEC] = {
    irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7,
    irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15,
    irq16, irq17, irq18, irq19, irq20, irq21, irq22, irq23,
    irq24, irq25, irq26, irq27, irq28, irq29, irq30, irq31 };

/* static int count = 0; */

struct gate_desc idt[IDT_ENTRIES] = { 0 };

void keyboard_hdl(void){
    uint8_t status;
    key_event ev;

    status = inb(KEYBOARD_STATUS_PORT);
    /* Lowest bit of status will be set if buffer is not empty */
    if (status & 0x01) {
        scancode_byte ks = inb(KEYBOARD_DATA_PORT);
        ev = keyboard_update_state(ks);
        tty_input(ks, ev);
    }
    write_eoi();
}


void common_hdl(void) {
    // TODO : something
}

void pit_hdl(void) {
    if (is_sleep && counter-- <= 0) {
        end_sleep();
    }
    write_eoi();
}

static inline void idt_int_asgn(int n, uint64_t addr) {
    idt[n].reserved    = 0;
    idt[n].offset_low  = (uint16_t)addr & 0xffff;
    idt[n].offset_mid  = (uint16_t)(addr >> 16) & 0xffff;
    idt[n].offset_high = (uint32_t)(addr >> 32);
    idt[n].segment     = KERNEL_SEGMENT_OFFSET;
    idt[n].ist         = 0;
    idt[n].type_attr   = INT_GATE;
}

void idt_init(void) {
    // --PIC remapping--
    // start init seq
    outb(PIC1_PORT, PIC_INIT_CODE);
    outb(PIC2_PORT, PIC_INIT_CODE);

    // change vector offset
    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);

    // purple magic
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    // -- PIT init --
    outb(PIT_CONF_PORT,  PIT_MODE);
    outb(PIT_DATA_PORT0, 0);
    outb(PIT_DATA_PORT0, 0);

    // -- masks --
    outb(PIC1_DATA,0xfc);
    outb(PIC2_DATA,0xff);

    // handlers for exceptions interruptions
    for (uint8_t n = 0; n < NEXCEPTION_VEC; n++)
        idt_int_asgn(n, (uint64_t)int_handlers[n]);

    idt_int_asgn(SYSCALL_VEC,  (uint64_t)irq_sys);
    idt_int_asgn(PIT_VEC,      (uint64_t)irq_pit);
    idt_int_asgn(KEYBOARD_VEC, (uint64_t)irq_keyboard);

    struct idt_reg reg;
    reg.limit = IDT_ENTRIES * (sizeof(struct gate_desc)) - 1;
    reg.base  = idt;
    asm volatile("lidt %0; sti;"::"m" (reg) : "memory");
}
