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
    iretq

irq_keyboard:
    call keyboard_hdl
    iretq

irq_sys:
    call syscall_hdl
    iretq

kernel_entry:
    mov %esp, %eax
    xorq %rsp, %rsp
    mov %eax, %esp

    call idt_init
    call kernel_main

    /*infinite loop*/
    cli
1:  hlt
    jmp 1b
