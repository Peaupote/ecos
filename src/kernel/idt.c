#include "idt.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "int.h"
#include "../util/vga.h"
#include "kmem.h"

extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);
extern void irq16(void);
extern void irq17(void);
extern void irq18(void);
extern void irq19(void);
extern void irq20(void);
extern void irq21(void);
extern void irq22(void);
extern void irq23(void);
extern void irq24(void);
extern void irq25(void);
extern void irq26(void);
extern void irq27(void);
extern void irq28(void);
extern void irq29(void);
extern void irq30(void);
extern void irq31(void);
extern void irq_sys(void);
extern void irq_keyboard(void);

const idt_handler handlers[NEXCEPTION_VEC] = {
    irq0,  irq1,  irq2,  irq3,
    irq4,  irq5,  irq6,  irq7,
    irq8,  irq9,  irq10, irq11,
    irq12, irq13, irq14, irq15,
    irq15, irq17, irq18, irq19,
    irq20, irq21, irq22, irq23,
    irq24, irq25, irq26, irq27,
    irq28, irq29, irq30, irq31
};

struct gate_desc idt[IDT_ENTRIES] = { 0 };
struct idt_reg reg;
unsigned char keyboard_map[128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
    '9', '0', '-', '=', '\b',	/* Backspace */
    '\t',			/* Tab */
    'q', 'w', 'e', 'r',	/* 19 */
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
    '\'', '`',   0,		/* Left shift */
    '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
    'm', ',', '.', '/',   0,				/* Right shift */
    '*',
    0,	/* Alt */
    ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
    '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
    '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

void keyboard_hdl(void) {
    printf("key\n");
    write_eoi();
    return;

    uint8_t status;
    char keycode;

    /* write EOI */
    // outb(0x20, 0x20);

    status = inb(KEYBOARD_STATUS_PORT);
    /* Lowest bit of status will be set if buffer is not empty */
    if (status & 0x01) {
        keycode = inb(KEYBOARD_DATA_PORT);
        if(keycode < 0)
            return;

        if(keycode == ENTER_KEY_CODE) {
            putchar('\n');
            return;
        }

        unsigned char c = keyboard_map[(unsigned char) keycode];
        putchar(c);
    }
}

void common_hdl(uint64_t code) {
    // TODO : print code
    printf("hdl\n");
}

void syscall_hdl(void) {
    printf("syscall\n");
}

void idt_init(void) {
    uint_ptr idt_addr = (uint_ptr)idt;
    uint64_t addr;

    // PROBLEM HERE
    /* // start init seq */
    /* outb(PIC1_PORT, PIC_INIT_CODE); */
    /* outb(PIC2_PORT, PIC_INIT_CODE); */

    /* // change vector offset */
    /* outb(PIC1_DATA, 0x20); */
    /* outb(PIC2_DATA, 0x28); */

    /* // purple magic */
    /* outb(PIC1_DATA, 0x04); */
    /* outb(PIC2_DATA, 0x02); */
    /* outb(PIC1_DATA, 0x01); */
    /* outb(PIC2_DATA, 0x01); */
    /* outb(PIC1_DATA, 0x0); */
    /* outb(PIC2_DATA, 0x0); */

    // handlers for exceptions interruptions
    for (uint8_t n = 0; n < NEXCEPTION_VEC; n++) {
        addr = (uint64_t)handlers[n];
        idt[n].reserved    = 0;
        idt[n].offset_low  = (uint16_t)addr & 0xffff;
        idt[n].offset_mid  = (uint16_t)(addr >> 16) & 0xffff;
        idt[n].offset_high = (uint32_t)(addr >> 32);
        idt[n].segment     = KERNEL_SEGMENT_OFFSET;
        idt[n].ist         = 0;
        idt[n].type_attr   = INT_GATE;
    }

    // Sysycalls handler
    addr = (uint64_t)irq_sys;
    idt[SYSCALL_VEC].reserved    = 0;
    idt[SYSCALL_VEC].offset_low  = (uint16_t)addr & 0xffff;
    idt[SYSCALL_VEC].offset_mid  = (uint16_t)(addr >> 16) & 0xffff;
    idt[SYSCALL_VEC].offset_high = (uint32_t)(addr >> 32);
    idt[SYSCALL_VEC].segment     = KERNEL_SEGMENT_OFFSET;
    idt[SYSCALL_VEC].ist         = 0;
    idt[SYSCALL_VEC].type_attr   = INT_GATE;

    addr = (uint64_t)irq_keyboard;
    idt[KEYBOARD_VEC].reserved    = 0;
    idt[KEYBOARD_VEC].offset_low  = (uint16_t)addr & 0xffff;
    idt[KEYBOARD_VEC].offset_mid  = (uint16_t)(addr >> 16) & 0xffff;
    idt[KEYBOARD_VEC].offset_high = (uint32_t)(addr >> 32);
    idt[KEYBOARD_VEC].segment     = KERNEL_SEGMENT_OFFSET;
    idt[KEYBOARD_VEC].ist         = 0;
    idt[KEYBOARD_VEC].type_attr   = INT_GATE;

    reg.content[0] = IDT_ENTRIES * (sizeof(struct gate_desc)) - 1;
    for(uint8_t i=1;i<5;++i) {
        reg.content[i] = idt_addr & 0xffff;
        idt_addr = idt_addr >> 16;
    }

    asm volatile("lidt %0; sti;"::"m" (reg) : "memory");
}
