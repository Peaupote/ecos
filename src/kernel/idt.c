#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "int.h"

/**
 * IDT gate description
 */
struct gate_desc {
    uint16_t  offset_low;   // offset 0..15
    uint16_t  segment;      // code segment in GDT or LDT
    uint8_t   ist;
    uint8_t   type_attr;    // type and attributes
    uint16_t  offset_mid;   // offset 16..31
    uint32_t  offset_high;  // offset 32..64
    uint32_t  reserved;
};

struct idt_reg {
    uint16_t   limit;
    struct gate_desc *base;
};

extern char inb(uint16_t port);
extern void outb(uint16_t port, uint16_t data);
extern void irq_default(void);
extern void irq_sys(void);
extern void irq_keyboard(void);

typedef void (*idt_handler)(void);
const idt_handler handlers[NEXCEPTION_VEC] = {
    irq_default, irq_default, irq_default, irq_default,
    irq_default, irq_default, irq_default, irq_default,
    irq_default, irq_default, irq_default, irq_default,
    irq_default, irq_default, irq_default, irq_default,
    irq_default, irq_default, irq_default, irq_default,
    irq_default, irq_default, irq_default, irq_default,
    irq_default, irq_default, irq_default, irq_default,
    irq_default, irq_default, irq_default, irq_default
};

struct gate_desc idt[IDT_ENTRIES] = { 0 };
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
    uint8_t status;
    char keycode;

    /* write EOI */
    outb(0x20, 0x20);

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

void common_hdl(void) {
    printf("common hdl\n");
}

void syscall_hdl(void) {
    printf("syscall\n");
}

void idt_init(void) {
    uint64_t addr;

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

    struct idt_reg reg;
    reg.base = idt;
    reg.limit = IDT_ENTRIES * (sizeof(struct gate_desc)) - 1;

    asm volatile("lidt %0"::"m" (reg));
    asm volatile("sidt %0":"=m" (reg));
}
