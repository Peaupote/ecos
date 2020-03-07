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
#include "proc.h"
#include "kutil.h"

extern void irq_sys(void);
extern void irq_keyboard(void);
extern void irq_pit(void);

extern const idt_handler int_handlers[NEXCEPTION_VEC];

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


void common_hdl(uint8_t num, uint64_t errcode) {
    klogf(Log_error, "error", "id %d, code %d", num, errcode);
    while(1) halt();
    // TODO : something
}

void pit_hdl(void) {
    static uint8_t clock = 0;
    if ((clock++ & SCHED_FREQ) == 0) schedule_proc(0);

    lookup_end_sleep();
    write_eoi();
}

static inline void idt_int_asgn(int n, uint64_t addr, uint8_t attr) {
    idt[n].reserved    = 0;
    idt[n].offset_low  = (uint16_t)addr & 0xffff;
    idt[n].offset_mid  = (uint16_t)(addr >> 16) & 0xffff;
    idt[n].offset_high = (uint32_t)(addr >> 32);
    idt[n].segment     = KERNEL_SEGMENT_OFFSET;
    idt[n].ist         = 0;
    idt[n].type_attr   = attr;
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
        idt_int_asgn(n, (uint64_t)int_handlers[n], INT_GATE);

    idt_int_asgn(SYSCALL_VEC,  (uint64_t)irq_sys, INT_GATE | 0x60);
    idt_int_asgn(PIT_VEC,      (uint64_t)irq_pit, INT_GATE);
    idt_int_asgn(KEYBOARD_VEC, (uint64_t)irq_keyboard, INT_GATE);

    struct idt_reg reg;
    reg.limit = IDT_ENTRIES * (sizeof(struct gate_desc)) - 1;
    reg.base  = idt;
    asm volatile("lidt %0; sti;"::"m" (reg) : "memory");
}
