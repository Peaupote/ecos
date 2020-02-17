#include "int.h"

.section .text
.global kernel_entry

    .global outb
    .global inb
    .global irq_default
    .global irq_sys
    .global irq_keyboard
    .global load_idt
    .extern common_hdl
    .extern syscall_hdl

inb:
    mov     4(%esp), %edx
    in      %dx, %al
    ret

outb:
    mov     4(%esp), %edx
    mov     8(%esp), %al
    out     %al, %dx
    ret

irq_default:
    call common_hdl
    iret

irq_keyboard:
    call keyboard_hdl
    iret

irq_sys:
    call syscall_hdl
    iret

load_idt:
    lea  4(%esp), %edx
    lidt (%edx)
    sti  // turn on interrupt
    ret

kernel_entry:
    mov %esp, %eax
    xorq %rsp, %rsp
    mov %eax, %esp

    call terminal_initialize
    call idt_init
    call kernel_main

    /*infinite loop*/
    cli
1:  hlt
    jmp 1b
