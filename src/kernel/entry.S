.section .text
.global kernel_entry
.global irq_handlers
.global irq_sys
.global irq_keyboard
.extern common_hdl
.extern syscall_hdl
.extern keyboard_hdl

irq_handlers:
#include "irq_handlers.s"

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
