#ifndef _IDT_H
#define _IDT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "int.h"

//Interrupt
#define IDT_TYPE_INT          0x0e
//Trap
#define IDT_TYPE_TRAP         0x0f
//DPL
#define IDT_ATTR_DPL(R)       ((R)<<5)
//Present
#define IDT_ATTR_P            (1<<7)

extern uint8_t write_eoi1_on_iret;

/**
 * IDT gate description
 */
struct gate_desc {
    uint16_t  offset_low;   // offset 0..15
    uint16_t  segment;      // code segment in GDT or LDT
    uint8_t   ist;			// [0..2] entry in IST
    uint8_t   type_attr;    // type and attributes
    uint16_t  offset_mid;   // offset 16..31
    uint32_t  offset_high;  // offset 32..64
    uint32_t  reserved;
} __attribute__((packed));

struct idt_reg {
    uint16_t   limit;
    struct gate_desc *base;
} __attribute__((packed));

typedef void (*idt_handler)(void);
const idt_handler int_handlers[NEXCEPTION_VEC];

void idt_init(void);

#endif
