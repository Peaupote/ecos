#include "int.h"

.section .text
.global kernel_entry
.global outb
.global inb
.global irq_default
.global irq_sys
.global irq_keyboard

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
    /*TEST: infinite loop pour eviter de crasher TODO*/
    cli
1:  hlt
    jmp 1b
    call common_hdl
    iretq

irq_keyboard:
    call keyboard_hdl
    iretq

irq_sys:
    call syscall_hdl
    iretq

kernel_entry:
    //On charge la nouvelle adresse virtuelle de la pile
    movl $stack_top_1, %esp
    shlq $32, %rsp
    movl $stack_top_0, %esp

    call kernel_main

    /*infinite loop*/
    cli
1:  hlt
    jmp 1b
