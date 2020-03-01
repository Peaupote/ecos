#define ASSEMBLY
#include "int.h"

    .section .text
.global irq_handlers
.global irq_sys
.global irq_keyboard
.global irq0
.extern common_hdl
.extern keyboard_hdl
.extern syscall_hdl

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

irq_sys:
    cli
    save_c_reg
    leaq -8(%rsp), %rsp //Alignement 16 octets
    call syscall_hdl
    leaq 8(%rsp), %rsp
    restore_c_reg
    sti
    iretq
