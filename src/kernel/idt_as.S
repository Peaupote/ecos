#define ASM_FILE
#include "int.h"
#include "sys.h"

    .section .data
    .global int_handlers
    .global proc_reg
.align 0x20
int_handlers:
#include "int_handlers_tab.s"

syscall_tab:
    .quad sleep
    .quad fork
    .quad kexit
    .quad wait
    .quad waitpid
    .quad getpid
    .quad getppid
    .quad open
    .quad close
    .quad dup
    .quad pipe
    .quad write
    .quad read
    .quad lseek

    .section .text
.global irq_sys
.global irq_keyboard
.global irq_pit
.global iret_to_userspace
.global eoi_iret_to_userspace
.global pml4_to_cr3
.global invalide_page

.extern st_curr_reg
.extern common_hdl
.extern keyboard_hdl
.extern syscall_hdl
.extern sleep_hdl
.extern kexit
.extern invalid_syscall

// Save all registers to be easily accessible in C code
.macro save_curr_proc_reg
    pushq  %r15
    movabs $st_curr_reg, %r15
    movq   (%r15), %r15
    movq   %rax, 24(%r15)
    movq   %rbx, 32(%r15)
    movq   %rcx, 40(%r15)
    movq   %rdx, 48(%r15)
    movq   %rsi, 56(%r15)
    movq   %rdi, 64(%r15)
    movq   %rbp, 72(%r15)
    movq   %r8,  80(%r15)
    movq   %r9,  88(%r15)
    movq   %r10, 96(%r15)
    movq   %r11, 104(%r15)
    movq   %r12, 112(%r15)
    movq   %r13, 120(%r15)
    movq   %r14, 128(%r15)
    movq   %r15, %rcx
    popq   %r15
    movq   %r15, 136(%rcx)

    pushfq // RFLAGS
    movq   (%rsp), %rbx
    movq   %rbx, (%rcx)
    popq   %rbx
    movq   (%rsp), %rbx // save rip
    movq   %rbx, 8(%rcx)
    movq   %rsp, 16(%rcx) // save rsp
.endm

.macro restore_curr_proc_reg
    movabs $st_curr_reg, %r15
    movq   (%r15), %r15
    movq   24(%r15), %rax
    movq   32(%r15), %rbx
    movq   40(%r15), %rcx
    movq   48(%r15), %rdx
    movq   56(%r15), %rsi
    movq   64(%r15), %rdi
    movq   72(%r15), %rbp
    movq   80(%r15), %r8
    movq   88(%r15), %r9
    movq   96(%r15), %r10
    movq   104(%r15), %r11
    movq   112(%r15), %r12
    movq   120(%r15), %r13
    movq   128(%r15), %r14
    pushq  %rax
    movq   %r15, %rax
    movq   136(%rax), %r15
    popq   %rax
.endm

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
    save_curr_proc_reg
    save_c_reg
    mov   %ds, %bx
    pushq %rbx
    mov   $0x10, %bx
    set_data_segments

    cmpq   $NSYSCALL, %rax
    jns    incorrect_rax

    movabs $syscall_tab, %rbx
    lea    (%rbx, %rax, 8), %rax
    movq   (%rax), %rax
    callq  *%rax
    jmp   func_end

incorrect_rax:
    call invalid_syscall

func_end:
    popq %rbx
    set_data_segments
    restore_c_reg
    restore_curr_proc_reg
    sti
    iretq

// restore registers from global struct st_curr_reg
.macro setup_iret
    movabs $st_curr_reg, %rax
    movq  (%rax), %rax
    mov   $0x23, %bx
    set_data_segments
    pushq $0x23    //SS
    pushq 16(%rax) //RSP
    pushq (%rax)   //RFLAGS
    popq  %rsi
    btsq  $9, %rsi //IF: interrupt flag
    pushq %rsi
    pushq $0x1B    //CS
    pushq 8(%rax)  //RIP
.endm

iret_to_userspace:
    setup_iret
    iretq

eoi_iret_to_userspace:
    setup_iret
    movb $PIC_EOI_CODE, %al
    outb %al, $PIC1_PORT
    restore_curr_proc_reg
    iretq

pml4_to_cr3:
    movq %rdi, %cr3
    ret

invalide_page:
	invlpg (%rdi)
	ret
