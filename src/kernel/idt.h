#ifndef _IDT_H
#define _IDT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

//Attention au padding
struct idt_reg {
	uint16_t content[5];
//  uint16_t   limit;
//  struct gate_desc *base;
};

typedef void (*idt_handler)(void);

void idt_init(void);

#endif
