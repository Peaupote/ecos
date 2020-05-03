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


#define VGA_BUFFER 0xB8000

// IRETQ structure
#define IRETQ_SS     0x20
#define IRETQ_RSP    0x18
#define IRETQ_RFLAGS 0x10
#define IRETQ_CS     0x08
#define IRETQ_RIP    0x00

// PIT
#define PIT_CHAN(I)  ((I)<< 6)
#define PIT_LOBYTE   (1 << 4)
#define PIT_HIBYTE   (1 << 5)
#define PIT_SQRGEN   (3 << 1)
#define PIT_FREQ0    1193182 

#endif
