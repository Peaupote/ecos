#ifndef _H_INT
#define _H_INT

// number of IDT vector numbers
#define IDT_ENTRIES 256

// number of vector numbers considered as exceptions raised by CPU
#define NEXCEPTION_VEC 32

// each IDT entry is 8 bytes long
#define IDT_HANDLER_SZ 9

// magic numbers
#define KERNEL_SEGMENT_OFFSET 0x08
#define INT_GATE              0x8e
#define INT_TRAP              0xf
#define INT_CALL              0xc
#define INT_TASK              0x5

// special interrupt vectors
#define SYSCALL_VEC           0x80
#define PIT_VEC               0x20
#define KEYBOARD_VEC          0x21

#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_DATA_PORT   0x60
#define ENTER_KEY_CODE 0x1C

#define PIC1_PORT 0x20
#define PIC1_DATA 0x21
#define PIC2_PORT 0xA0
#define PIC2_DATA 0xA1

#define PIC_INIT_CODE 0x11 // initialisation
#define PIC_EOI_CODE  0x20 // end-of-interrupt

// PIT = Programmable Interval Timer
#define PIT_DATA_PORT0 0x40
#define PIT_DATA_PORT1 0x41
#define PIT_DATA_PORT2 0x42
#define PIT_CONF_PORT  0x43
#define PIT_MODE       0b00110110

#ifndef ASSEMBLY

#include <stddef.h>
#include <stdint.h>

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t data) {
    asm volatile ("outb %1, %0"::"dN"(port), "a"(data));
}

static inline void io_wait(void) {
    // must be unused port
    asm volatile ("outb %%al, $0x80" : : "a"(0) );
}

static inline void write_eoi(void) {
    outb(PIC1_PORT, PIC_EOI_CODE);
}

static inline void clear_interrupt_flag(void) {
    asm volatile ("cli");
}

static inline void set_interrupt_flag(void) {
    asm volatile ("sti");
}

static inline void halt(void) {
    asm volatile ("hlt");
}

static inline void int_syscall(int code) {
    asm volatile ("mov %0, %%rax; int $0x80" :: "dN"(code) : "rax", "memory");
}

#endif

#endif
