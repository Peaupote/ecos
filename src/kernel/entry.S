#define ASSEMBLY
#include "int.h"

    .section .data
.global test_count
.global test_data_in
test_count:
    .quad 0
test_data_in:
    .byte 0

    .section .text
.global kernel_entry
.global irq_handlers
.global irq_sys
.global irq_keyboard
.extern common_hdl
.extern syscall_hdl
.extern keyboard_hdl

int_handlers:
#include "int_handlers.s"

irq_keyboard:
    pushq %rax
    pushq %rdi
    movabsq $test_count, %rdi
    movq $0x10000000, (%rdi)

//    call keyboard_hdl
    inb $KEYBOARD_STATUS_PORT, %al
    testb 0x01, %al
    je statut_0
    inb $KEYBOARD_DATA_PORT, %al
    movabsq $test_data_in, %rdi
    movb %al, (%rdi)
statut_0:
    mov $PIC_EOI_CODE, %al
    outb %al, $PIC1_PORT
    popq %rdi
    popq %rax
    iretq

irq_sys:
    call syscall_hdl
    iretq

kernel_entry:
    //On charge la nouvelle adresse virtuelle de la pile
    movabsq $stack_top, %rsp
    
    call kernel_main

    /*infinite loop*/
    cli
1:  hlt
    jmp 1b
