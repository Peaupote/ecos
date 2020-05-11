#ifndef _HWD_H
#define _HWD_H

// --Control Registers--

//Write Protect
#define CR0_WP 16
//Paging
#define CR0_PG 31

//Page Size Extensions
#define CR4_PSE 4
//Physical Address Extension
#define CR4_PAE 5
//Page Global Enable
#define CR4_PGE 7

// --RFLAGS Register--
//Interrupt Enable Flag
#define RFLAGS_IF 9

// --Model Specific Registers--

//Extended Feature Enables
#define MSR_EFER     0xC0000080
//Long Mode
#define MSR_EFER_LME 8


#define VGA_BUFFER   0xB8000

// IRETQ structure
#define IRETQ_SS     0x20
#define IRETQ_RSP    0x18
#define IRETQ_RFLAGS 0x10
#define IRETQ_CS     0x08
#define IRETQ_RIP    0x00

// PIC

#define PIC1_IRQ_PIT  0x00
// keyboard
#define PIC1_IRQ_KEYB 0x01
// slave PIC
#define PIC1_IRQ_PIC2 0x02
// spurious
#define PIC1_IRQ_SPUR 0x07

#define PIC1_PORT     0x20
#define PIC1_DATA     0x21
#define PIC2_PORT     0xA0
#define PIC2_DATA     0xA1

// Slave ident
#define PIC2_IDENT    0x02

// initialization command words
#define PIC_ICW1_INIT 0x10
#define PIC_ICW1_ICW4 0x01

#define PIC_ICW4_8086 0x01

// end-of-interrupt
#define PIC_EOI_CODE  0x20


#define KEYBOARD_STATUS_PORT  0x64
#define KEYBOARD_DATA_PORT    0x60


// PIT: Programmable Interval Timer
#define PIT_CHAN(I) ((I)<< 6)
#define PIT_LOBYTE  (1 << 4)
#define PIT_HIBYTE  (1 << 5)
#define PIT_SQRGEN  (3 << 1)
#define PIT_FREQ0   1193182 

#define PIT_DATA_PORT0        0x40
#define PIT_DATA_PORT1        0x41
#define PIT_DATA_PORT2        0x42
#define PIT_CONF_PORT         0x43

#endif
