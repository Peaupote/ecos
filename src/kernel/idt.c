#include "idt.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "int.h"
#include "../util/vga.h"
#include "../util/string.h"
#include "kmem.h"
#include "keyboard.h"
#include "tty.h"

extern const idt_handler int_handlers[NEXCEPTION_VEC];
extern void irq_sys(void);
extern void irq_keyboard(void);

struct gate_desc idt[IDT_ENTRIES] = { 0 };
void keyboard_hdl(void){
    uint8_t status;

    status = inb(KEYBOARD_STATUS_PORT);
    /* Lowest bit of status will be set if buffer is not empty */
    if (status & 0x01) {
        keyboard_input_keycode = inb(KEYBOARD_DATA_PORT);
		keyboard_update_state(keyboard_input_keycode);
		tty_input(keyboard_input_keycode);
	}
	write_eoi();
}

char code_str[17] = "__..__..__..__..";

void common_hdl(uint64_t code) {
    int64_to_str_hexa(code_str, code);
    printf("hdl:");
    printf(code_str);
    printf("\n");
}

void syscall_hdl(void) {
    printf("syscall\n");
}

void idt_init(void) {
    uint64_t addr;

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

    //masks: keyboard only
    outb(PIC1_DATA,0xfd);
    outb(PIC2_DATA,0xff);

    // handlers for exceptions interruptions
    for (uint8_t n = 0; n < NEXCEPTION_VEC; n++) {
        addr = (uint64_t)int_handlers[n];
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
    reg.limit = IDT_ENTRIES * (sizeof(struct gate_desc)) - 1;
    reg.base  = idt;
    asm volatile("lidt %0; sti;"::"m" (reg) : "memory");
}
