#include <kernel/idt.h>

#include <stddef.h>
#include <stdint.h>

#include <kernel/int.h>
#include <kernel/gdt.h>
#include <kernel/sys.h>
#include <kernel/memory/kmem.h>
#include <kernel/memory/shared_pages.h>
#include <kernel/keyboard.h>
#include <kernel/tty.h>
#include <kernel/proc.h>
#include <kernel/kutil.h>

#include <util/vga.h>
#include <util/string.h>

#define GATE_INT  (IDT_ATTR_P|IDT_TYPE_INT)

extern void irq_sys(void);
extern void irq_sys_ring1(void);
extern void irq_sys_ring1_call(void);
extern void irq_keyboard(void);
extern void irq_pit(void);

extern const idt_handler int_handlers[NEXCEPTION_VEC];

struct gate_desc idt[IDT_ENTRIES] = { 0 };

// ! Processus partiellement sauvegardé
void keyboard_hdl(void){
    uint8_t status;
    key_event ev;

    status = inb(KEYBOARD_STATUS_PORT);
    /* Lowest bit of status will be set if buffer is not empty */
    if (status & 0x01) {
        scancode_byte ks = inb(KEYBOARD_DATA_PORT);
        ev = keyboard_update_state(ks);
        tty_input(ks, ev);
    }
    write_eoi();
}

#define NEXCEPTION 22
const char *error_desc[NEXCEPTION + 1] = {
    "Division by zero",
    "Debug exception",
    "NMI interrupt",
    "Breakpoint exception",
    "Overflow exception",
    "Bound range exception overflow",
    "Invalid opcode exception",
    "Device not available exception",
    "Double fault exception",
    "Coprocessor segment overrun",
    "Invalid TSS exception",
    "Segment not present",
    "Stack fault exception",
    "General protection exception",
    "Page fault exception",
    "Floating point error",
    "Alignment check exception",
    "Machine check exception",
    "SIMD Foating point exception",
    "Virtualization exception",
    "Control protection exception",
    "Unknown exception"
};

void common_hdl(uint8_t num, uint64_t errcode) {
    // TODO : something
    const char *desc = error_desc[num < NEXCEPTION ? num : NEXCEPTION];
    kpanicf("exception", "%s: error code %llx", desc, errcode);
}


// Processus partiellent sauvegardé
bool pit_hdl(void) {
    static uint8_t clock = 0;
    lookup_end_sleep();
    if ((clock++ & SCHED_FREQ) == 0) {
        pid_t pid = schedule_proc_ev();
        if (pid != state.st_curr_pid) {
            state.st_curr_pid = pid;
            return true;
        }
    }

    write_eoi();
    return false;
}

void pit_hdl_switch(void) {
    proc_t *p = switch_proc(state.st_curr_pid);
    klogf(Log_verb, "sched",
          "pit switch, nb R %d, run proc %d :\n"
          "   rip %p, rsp %p",
          state.st_sched.nb_proc + 1, state.st_curr_pid,
          p->p_reg.rip, p->p_reg.rsp);
    eoi_run_proc(p);
}

//--#PF errcode--
//Present
#define EXC_PF_ERC_P 1
//Write
#define EXC_PF_ERC_W (1<<1)
//User
#define EXC_PF_ERC_U (1<<2)

// ! Processus partiellement sauvegardé
void exception_PF_hdl(uint_ptr fault_vaddr, uint64_t errcode) {
    klogf(Log_verb, "exc", "#PF on %p, errcode=%llx",
            fault_vaddr, errcode);
    if (!(errcode & EXC_PF_ERC_P)
            && paging_get_lvl(pgg_pml4, fault_vaddr) < PML4_END_USPACE) {
        if (handle_PF(fault_vaddr))
            kpanicf("#PF handling", "on %p errcode=%llx",
                    fault_vaddr, errcode);
    } else //TODO: kill process
        kpanicf("#PF not handled", "on %p errcode=%llx",
                    fault_vaddr, errcode);
    klogf(Log_verb, "exc", "#PF handled");
}

static inline void idt_int_asgn(int n, uint64_t addr, uint8_t attr,
        uint8_t ist) {
    idt[n].reserved    = 0;
    idt[n].offset_low  = (uint16_t)addr & 0xffff;
    idt[n].offset_mid  = (uint16_t)(addr >> 16) & 0xffff;
    idt[n].offset_high = (uint32_t)(addr >> 32);
    idt[n].segment     = SEG_SEL(GDT_RING0_CODE, 0);
    idt[n].ist         = ist;
    idt[n].type_attr   = attr;
}

void idt_init(void) {
    // --PIC remapping--
    // start init seq
    outb(PIC1_PORT, PIC_INIT_CODE);
    outb(PIC2_PORT, PIC_INIT_CODE);

    // change vector offset
    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);

    // purple magic
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    // -- PIT init --
    outb(PIT_CONF_PORT,  PIT_MODE);
    outb(PIT_DATA_PORT0, 0);
    outb(PIT_DATA_PORT0, 0);

    // -- masks --
    outb(PIC1_DATA,0xfc);
    outb(PIC2_DATA,0xff);

    // handlers for exceptions interruptions
    for (uint8_t n = 0; n < NEXCEPTION_VEC; n++)
        idt_int_asgn(n, (uint64_t)int_handlers[n], GATE_INT, 0);

    idt_int_asgn(SYSCALL_VEC, (uint64_t)irq_sys,
            GATE_INT | IDT_ATTR_DPL(3), 0);
    idt_int_asgn(SYSCALL_R1_VEC, (uint64_t)irq_sys_ring1,
            GATE_INT | IDT_ATTR_DPL(1), 0);
    idt_int_asgn(SYSCALL_R1_CALL_VEC, (uint64_t)irq_sys_ring1_call,
            GATE_INT | IDT_ATTR_DPL(1), 0);
    idt_int_asgn(PIT_VEC,      (uint64_t)irq_pit,      GATE_INT, 0);
    idt_int_asgn(KEYBOARD_VEC, (uint64_t)irq_keyboard, GATE_INT, 0);

    struct idt_reg reg;
    reg.limit = IDT_ENTRIES * (sizeof(struct gate_desc)) - 1;
    reg.base  = idt;
    asm volatile("lidt %0; sti;"::"m" (reg) : "memory");
}
