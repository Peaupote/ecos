#define ASSEMBLY
#include "int.h"
#include "sys.h"

    .section .data
.global int_handlers
.align 0x20
int_handlers:
#include "int_handlers_tab.s"

    .section .text
.global irq_sys
.global irq_keyboard
.global irq_pit
.global iret_to_userspace

.extern common_hdl
.extern keyboard_hdl
.extern syscall_hdl
.extern sleep_hdl

//Sauvegarde des registres pouvant être modifiés par un appel à une
//fonction C. Place 9*8 octets sur la pile
.macro save_c_reg
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
.endm
.macro restore_c_reg
    popq  %r11
    popq  %r10
    popq  %r9
    popq  %r8
    popq  %rdi
    popq  %rsi
    popq  %rdx
    popq  %rcx
    popq  %rax
.endm

.macro set_data_segments
    mov %bx, %ds
    mov %bx, %es
    mov %bx, %fs
    mov %bx, %gs
.endm

#include "int_handlers.s"

irq_keyboard:
    cli
    save_c_reg
    leaq -8(%rsp), %rsp //Alignement 16 octets
    call keyboard_hdl
    leaq 8(%rsp), %rsp
    restore_c_reg
    sti
    iretq

irq_pit:
    cli
    save_c_reg
    leaq -8(%rsp), %rsp //Alignement 16 octets
    call pit_hdl
    leaq 8(%rsp), %rsp
    restore_c_reg
    sti
    iretq

irq_sys:
    cli
    save_c_reg
    mov   %ds, %bx
    pushq %rbx
    mov   $0x10, %bx
    set_data_segments

    // TODO : implement switch on %rax values
    cmpq $0x42, %rax
    je test_proc_int

    call sleep_hdl

    popq %rbx
    set_data_segments
    restore_c_reg
    sti
    iretq

//arg: RIP, RSP
iret_to_userspace:
    mov   $0x23, %bx
    set_data_segments
    pushq $0x23 //SS
    pushq %rsi  //RSP
    pushfq      //RFLAGS
    pushq $0x1B //CS
    pushq %rdi  //RIP
    iretq

test_proc_int:
    movq %rsp, %rsi
    call tty_test_prg_rt
    sti
lh:	hlt
    jmp lh
