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
    asm volatile ( "outb %%al, $0x80" : : "a"(0) );
}

static inline void write_eoi(void) {
    outb(PIC1_PORT, PIC_EOI_CODE);
}

#endif
