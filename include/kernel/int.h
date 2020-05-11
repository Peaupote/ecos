#ifndef _H_INT
#define _H_INT

#include <util/hdw.h>

// number of IDT vector numbers
#define IDT_ENTRIES            256

// number of vector numbers considered as exceptions raised by CPU
#define NEXCEPTION_VEC          32

#define PIC1_OFFSET           0x20
#define PIC2_OFFSET           0x28

// special interrupt vectors
#define SYSCALL_VEC           0x80
#define SYSCALL_R1_VEC        0x7f
#define SYSCALL_R1_CALL_VEC   0x7e
#define PIT_VEC               (PIC1_OFFSET + PIC1_IRQ_PIT )
#define SPURIOUS_VEC          (PIC1_OFFSET + PIC1_IRQ_SPUR)
#define KEYBOARD_VEC          (PIC1_OFFSET + PIC1_IRQ_KEYB)

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
