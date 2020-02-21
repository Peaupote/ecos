#ifndef _IDT_H
#define _IDT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
}
//Supprime le padding, rend l'accès plus lent 
//mais devrais fonctionner en x86
__attribute__((packed));

typedef void (*idt_handler)(void);
const idt_handler irq_handlers[NEXCEPTION_VEC];

void idt_init(void);

#endif
