.section .text
.global kernel_entry
.global irq0
.global irq1
.global irq2
.global irq3
.global irq4
.global irq5
.global irq6
.global irq7
.global irq8
.global irq9
.global irq10
.global irq11
.global irq12
.global irq13
.global irq14
.global irq15
.global irq16
.global irq17
.global irq18
.global irq19
.global irq20
.global irq21
.global irq22
.global irq23
.global irq24
.global irq25
.global irq26
.global irq27
.global irq28
.global irq29
.global irq30
.global irq31
.global irq_sys
.global irq_keyboard
.extern common_hdl
.extern syscall_hdl
.extern keyboard_hdl

// TODO : something cleaner with .rept
irq0:
    movq $0, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq1:
    movq $1, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq2:
    movq $2, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq3:
    movq $3, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq4:
    movq $4, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq5:
    movq $5, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq6:
    movq $6, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq7:
    movq $7, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq8:
    movq $8, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq9:
    movq $9, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq10:
    movq $10, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq11:
    movq $11, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq12:
    movq $12, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq13:
    movq $13, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq14:
    movq $14, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq15:
    movq $15, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq16:
    movq $16, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq17:
    movq $17, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq18:
    movq $18, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq19:
    movq $19, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq20:
    movq $20, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq21:
    movq $21, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq22:
    movq $22, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq23:
    movq $23, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq24:
    movq $24, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq25:
    movq $25, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq26:
    movq $26, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq27:
    movq $27, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq28:
    movq $28, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq29:
    movq $29, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq30:
    movq $30, %rdi
    call common_hdl
    add $8, %rsp
    iretq

irq31:
    movq $31, %rdi
    call common_hdl
    add $8, %rsp
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
