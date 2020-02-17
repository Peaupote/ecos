// number of IDT vector numbers
#define IDT_ENTRIES 256

// number of vector numbers considered as exceptions raised by CPU
#define NEXCEPTION_VEC 32

// each IDT entry is 8 bytes long
#define IDT_HANDLER_SZ 9

#define KERNEL_SEGMENT_OFFSET 0x08
#define INT_GATE              0x8e
#define SYSCALL_VEC           0x80
#define KEYBOARD_VEC          0x21

#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_DATA_PORT   0x60
#define ENTER_KEY_CODE 0x1C
