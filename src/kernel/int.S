#define ASM_FILE
#include <kernel/int.h>
#include <kernel/sys.h>
#include <kernel/gdt.h>
#include <util/hdw.h>

    .section .data
    .global int_handlers
    .global proc_reg
.align 0x20
int_handlers:
#include "int_handlers_tab.s"

syscall_tab:
    .quad sys_usleep
    .quad sys_fork
    .quad sys_exit
    .quad sys_wait
    .quad sys_waitpid
    .quad sys_getpid
    .quad sys_getppid
    .quad sys_open
    .quad sys_close
    .quad sys_dup
    .quad sys_dup2
    .quad sys_pipe
    .quad sys_write
    .quad sys_read
    .quad sys_lseek
    .quad sys_execve
    .quad sys_fstat
    .quad sys_mkdir
    .quad sys_chdir
    .quad sys_setpriority
    .quad sys_getpriority
    .quad sys_sbrk
    .quad sys_sigsethnd
    .quad sys_sigreturn
    .quad sys_signal
    .quad sys_kill
    .quad sys_errno
    .quad sys_pause
    .quad sys_link
    .quad sys_symlink
    .quad sys_readlink
syscall_ring1j_tab:
    .quad sys_prires
syscall_ring1c_tab:
    .quad tty_set_owner

    .section .text
.global irq_sys
.global continue_syscall
.global irq_sys_ring1
.global irq_sys_ring1_call
.global irq_keyboard
.global irq_pit
.global irq_spurious

.global iret_to_userspace

.global pml4_to_cr3
.global invalide_page
.global paging_refresh

.global kpanic_do_stop

.global schedule_proc

.extern st_curr_reg
.extern common_hdl
.extern keyboard_hdl
.extern syscall_hdl
.extern sleep_hdl
.extern kexit
.extern invalid_syscall
.extern exception_PF_hdl
.extern pit_hdl
.extern pit_hdl_switch
.extern panic_rsp
.extern _schedule_proc
.extern kernel_stack_top
.extern write_eoi1_on_iret

// Envoie EOI au PIC1, utilise %al
.macro write_eoi1
    movb $PIC_EOI_CODE, %al
    outb %al, $PIC1_PORT
.endm

// Sauvegarde des registres associés au processus courant (st_curr_reg)
// Utilise la valeur actuelle des registres ainsi qu'une structure d'iret
// placée sur la pile en %rsp+ofs
// Les registres autres que %r10 et %r11 ne sont pas modifiés
.macro save_curr_proc_reg ofs
    pushq  %r15
    movabs $st_curr_reg, %r15
    movq   (%r15), %r15
    movq   %rax, 0x18(%r15)
    movq   %rbx, 0x60(%r15)
    movq   %rcx, 0x20(%r15)
    movq   %rdx, 0x28(%r15)
    movq   %rsi, 0x30(%r15)
    movq   %rdi, 0x38(%r15)
    movq   %rbp, 0x68(%r15)
    movq   %r8,  0x40(%r15)
    movq   %r9,  0x48(%r15)
    movq   %r10, 0x50(%r15)
    movq   %r11, 0x58(%r15)
    movq   %r12, 0x70(%r15)
    movq   %r13, 0x78(%r15)
    movq   %r14, 0x80(%r15)
    movq   %r15, %r10
    popq   %r15
    movq   %r15, 0x88(%r10)

    // Sauvegarde des données présentes dans la structure d'iret
    // en %rsp+ofs
    movq   (IRETQ_RFLAGS + \ofs)(%rsp), %r11
    movq   %r11,  0x00(%r10) // reg.rflags
    movq   (IRETQ_RIP + \ofs)(%rsp),    %r11
    movq   %r11,  0x08(%r10) // reg.rip
    movq   (IRETQ_RSP + \ofs)(%rsp),    %r11
    movq   %r11,  0x10(%r10) // reg.rsp
.endm

.macro restore_curr_proc_reg_rbx_r15
    movq   0x60(%r15), %rbx
    movq   0x20(%r15), %rcx
    movq   0x28(%r15), %rdx
    movq   0x30(%r15), %rsi
    movq   0x38(%r15), %rdi
    movq   0x68(%r15), %rbp
    movq   0x40(%r15), %r8
    movq   0x48(%r15), %r9
    movq   0x50(%r15), %r10
    movq   0x58(%r15), %r11
    movq   0x70(%r15), %r12
    movq   0x78(%r15), %r13
    movq   0x80(%r15), %r14
    movq   0x88(%r15), %r15
.endm
.macro restore_curr_proc_reg
    movabs $st_curr_reg, %r15
    movq   (%r15), %r15
    movq   0x18(%r15), %rax
    restore_curr_proc_reg_rbx_r15
.endm

.macro restore_curr_proc_reg_exc_rax
    movabs $st_curr_reg, %r15
    movq   (%r15), %r15
    restore_curr_proc_reg_rbx_r15
.endm

// Charge à partir de st_curr_reg les registres pouvant transmettre des
// paramètres aux appels système
.macro load_curr_prog_regp
    movabs $st_curr_reg, %r15
    movq   (%r15), %r15
    movq   0x18(%r15), %rax
    movq   0x20(%r15), %rcx
    movq   0x28(%r15), %rdx
    movq   0x30(%r15), %rsi
    movq   0x38(%r15), %rdi
    movq   0x40(%r15), %r8
    movq   0x48(%r15), %r9
.endm

// Sauvegarde des registres pouvant être modifiés par un appel à une
// fonction C. Place 9*8 octets sur la pile
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
// Récupère le code d'erreur (8 octets) dans %rsi
.macro save_c_reg_err
    pushq %rcx
    pushq %rdx
    pushq %rsi
    movq  24(%rsp), %rsi
    movq  %rax, 24(%rsp)
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

// Utilise la valeur de %r10w comme selecteur pour les segments de donnée
.macro set_data_segments
    mov %r10w, %ds
    mov %r10w, %es
    mov %r10w, %fs
    mov %r10w, %gs
.endm

// Place 8 octets sur la pile
// Modifient %r10
.macro save_data_segments
    movw   %ds,   %r10w
    pushq  %r10
.endm
.macro restore_data_segments
    popq   %r10
    set_data_segments
.endm
// Place 16 octets sur la pile
// Ne modifient aucun registre
.macro save_data_segments16
    pushq %r10
    save_data_segments
    movq 8(%rsp), %r10
.endm
.macro restore_data_segments16
    movq  %r10, 8(%rsp)
    restore_data_segments
    popq  %r10
.endm

// Prépare une structure pour iretq
// %rsp, %rip à partir de st_curr_reg
// CS = %rdi, DS = %rdi + 8
// place le selecteur des DS dans %r10
.macro setup_iret
    movabs $st_curr_reg, %rax
    movq  (%rax), %rax
    leaq  8(%rdi), %r10
    pushq %r10             //SS
    pushq 0x10(%rax)       //RSP
    movq  0x00(%rax), %rsi //RFLAGS
    btsq  $RFLAGS_IF, %rsi
    pushq %rsi
    pushq %rdi             //CS
    pushq 0x08(%rax)       //RIP
.endm

// Utilise le selecteur du SS d'une structure d'iret placée sur la pile
// Ne modifie aucun registre
.macro set_data_segments_from_iret
    pushq %r10
    movq  0x28(%rsp), %r10 //SS
    set_data_segments
    popq  %r10
.endm

#include "int_handlers.s"

irq_keyboard:
    save_c_reg
    save_data_segments
    movw  $SEG_SEL(GDT_RING0_DATA,0), %r10w
    set_data_segments

    movabs $keyboard_hdl, %r10
    callq *%r10

    restore_data_segments
	write_eoi1
    restore_c_reg
    iretq

irq_pit:
    save_c_reg
    save_data_segments
    movw  $SEG_SEL(GDT_RING0_DATA,0), %r10w
    set_data_segments

    movabs $pit_hdl, %r10
    callq  *%r10
    testl  %eax, %eax
    je     irq_pit_remain

    movq   %rax, (%rsp)
    leaq   8(%rsp), %rsp
    restore_c_reg
    save_curr_proc_reg 0
    movq   -0x50(%rsp), %rdi //retour de pid_hdl
    movabs $pit_hdl_switch, %r10
    callq  *%r10

irq_pit_remain:
    restore_data_segments
	write_eoi1
    restore_c_reg
    iretq

irq_spurious:
	pushq  %r10
	pushq  %r11
	movabs $nb_spurious_irq, %r11
	movq   (%r11), %r10
	incq   %r10
	movq   %r10,  (%r11)
	popq   %r11
	popq   %r10
	// on n'envoie pas EOI
	iretq

irq_sys:
    save_data_segments16
    movw  $SEG_SEL(GDT_RING0_DATA,0), %r10w
    set_data_segments

    save_curr_proc_reg 0x10

    cmpq   $NSYSCALL, %rax
    jae    incorrect_rax

    movabs $syscall_tab, %r10
    movq   (%r10, %rax, 8), %r10
    callq  *%r10

func_end:
    restore_curr_proc_reg_exc_rax
    restore_data_segments16
    iretq

incorrect_rax:
    movabs $invalid_syscall, %r10
    callq  *%r10
    jmp    func_end

continue_syscall:
    load_curr_prog_regp
    movabs $syscall_tab, %r10
    movq   (%r10, %rax, 8), %r10
    jmpq   *%r10 //Appel terminal

irq_sys_ring1_call:
    callq  *%rax
    iretq

irq_sys_ring1:
    save_c_reg
    save_data_segments
    movw   $SEG_SEL(GDT_RING0_DATA,0), %r10w
    set_data_segments

    testq  $0x100, %rax
    jne    irq_sys_ring1c
    movabs $syscall_ring1j_tab, %r10
    movq   (%r10,  %rax, 8), %r10
    jmpq   *%r10
irq_sys_ring1c:
    movabs $syscall_ring1c_tab, %r10
    andq   $0xff,  %rax
    movq   (%r10,  %rax, 8), %r10
    callq  *%r10

irq_sys_ring1_end:
    restore_data_segments
    restore_c_reg
    iretq
//Pile:
// struct iret
// reg save 9 quad
// ds  save 1 quad
sys_prires:
    movq  $(GDT_RING_SZ+1), %r10 //+1 pour le CPL
    imulw %di,              %r10w
    addw  $GDT_RING_OFS,    %r10w
    movq  %r10, 0x58(%rsp) //cs
    addw  $8,               %r10w
    movq  %r10,     (%rsp) //ds
    movq  %r10, 0x70(%rsp) //ss
    call  prires_proc_struct
    jmp   irq_sys_ring1_end

irq14://#PF: Page Fault
    save_c_reg_err
    leaq   -8(%rsp), %rsp
    movq   %cr2, %rdi //Adresse provoquant le PF
    call   exception_PF_hdl
    leaq   8(%rsp), %rsp
    restore_c_reg
    iretq

iret_to_userspace:
    setup_iret
	movabs $write_eoi1_on_iret, %r10
	movb   (%r10), %al
	testb  %al, %al
	jne    eoi1_iret_to_userspace
    restore_curr_proc_reg
    set_data_segments_from_iret
    iretq

eoi1_iret_to_userspace:
	movb   $0, (%r10)
	write_eoi1
    restore_curr_proc_reg
    set_data_segments_from_iret
    iretq

pml4_to_cr3:
    movq %rdi, %cr3
    ret

invalide_page:
    invlpg (%rdi)
    ret

paging_refresh:
    movq   %cr3, %rax
    movq   %rax, %cr3
    ret

kpanic_do_stop:
    movabs $panic_rsp,   %r10
    movq   %rsp,        (%r10)
    movabs $st_curr_reg, %r10
    movq   (%r10),       %r10
    movq   %rsp,    0x10(%r10)//.rsp
    sti
1:  hlt
    jmp 1b

schedule_proc:
    movabs $kernel_stack_top, %rsp
    movabs $_schedule_proc,   %rax
    jmpq   *%rax
