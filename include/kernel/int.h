#ifndef _H_INT
#define _H_INT

#include <util/hdw.h>

// number of IDT vector numbers
#define IDT_ENTRIES 256

// number of vector numbers considered as exceptions raised by CPU
#define NEXCEPTION_VEC 32

// special interrupt vectors
#define SYSCALL_VEC           0x80
#define SYSCALL_R1_VEC        0x7f
#define SYSCALL_R1_CALL_VEC   0x7e
#define PIT_VEC               0x20
#define KEYBOARD_VEC          0x21

#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_DATA_PORT   0x60

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
#define PIT_MODE       (PIT_SQRGEN|PIT_LOBYTE|PIT_HIBYTE|PIT_CHAN(0))

#ifndef ASM_FILE

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

#endif

#endif
