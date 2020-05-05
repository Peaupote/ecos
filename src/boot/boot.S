#define ASM_FILE
#include <util/gdt.h>
#include <util/hdw.h>
#include <def.h>

/* From https://wiki.osdev.org/Bare_Bones */
/* Declare constants for the multiboot header. */
.set ALIGN,    1<<0             /* align loaded modules on page boundaries */
.set MEMINFO,  1<<1             /* provide memory map */
.set GRAPHIC,  1<<2             /* specify prefered mode */

.set GRAPHIC_LIN,  0
.set GRAPHIC_TEXT, 1

.set FLAGS,    ALIGN | MEMINFO | GRAPHIC
.set MAGIC,    0x1BADB002       /* 'magic number' lets bootloader find the header */
.set CHECKSUM, -(MAGIC + FLAGS) /* checksum of above, to prove we are multiboot */

/*
Declare a multiboot header that marks the program as a kernel. These are magic
values that are documented in the multiboot standard. The bootloader will
search for this signature in the first 8 KiB of the kernel file, aligned at a
32-bit boundary. The signature is in its own section so the header can be
forced to be within the first 8 KiB of the kernel file.
*/
    .section .multiboot
.balign 4
.long MAGIC
.long FLAGS
.long CHECKSUM
.skip 20
/* graphic mode */
#ifdef GRAPHIC_MODE_LIN
.long GRAPHIC_LIN
.long 0    /* width:  no preference   */
.long 0    /* height: no preference   */
.long 24   /* depth 24 bits per pixel */
#else
.long GRAPHIC_TEXT
.long 0    /* width:  no preference   */
.long 0    /* height: no preference   */
.long 0    /* depth:  no preference   */
#endif

    .section .bss

//Paging structures for id mapping
.global page_dpt_0
.global page_d_0

.global mb_info
.global kernel_entry_addr

//Structures pour le mapping identité sur les 4 premiers MB
.balign 0x1000
page_dpt_0: #PDPT
.skip   0x1000 # 512 * 64 bits

.balign 0x1000
page_d_0:
.skip   0x1000 # 512 * 64 bits

.balign 4
mb_info:
.skip   4
kernel_entry_addr: //adresse physique
.skip   4

//espace pour le kernel
//évite que GRUB charge le module sur une adresse utilisée
    .section space_for_kernel
.global KPA           //Adresse physique du kernel
.global KPA_end
.balign 0x1000        //alignement à la page 4KB
KPA: .skip 0x300000   //3MB
KPA_end:


    .section .text
.global _start
.type _start, @function
_start:

    //On suppose que l'on est démarré par GRUB
    //Mode protégé avec des segments 'plats'
    //Paging désactivé
    //Ligne A20 activée

    //On récupère les informations de multiboot
    //https://www.gnu.org/software/grub/manual/multiboot/html_node/Boot-information-format.html
    movl %ebx, (mb_info)

    //Stack
    mov $stack_top, %esp

    call init_paging_structs

/*	movl (mb_info), %ebx
    movl 72(%ebx),  %ebx

	movw 14(%ebx),   %ax
	movl (mb_info), %ebx
	movw %ax, 80(%ebx)*/

/*	movw %ds, %cx
	ldsw 14(%ebx),   %bx

	movl $0, %eax
	jmp  loop_test
loop_bg:
	leaw 2(%bx), %bx
	incl %eax
loop_test:
	mov  %bx, %dx
	cmpw 0xFFFF, %dx
	jne  loop_bg

	movw %cx, %ds*/

    /*
     On charge le kernel ici afin d'être sûr de pouvoir accéder
     aux données fournies par GRUB
     On récupère l'adresse du kernel 64 bits dans kernel_entry_addr
    */
    call load_kernel64

    /* --GDT-- */
#include "gdt.s"
    /* GDT descriptor */
    movw $(4*8 - 1), gdt_desc
    movl $gd_table,  (gdt_desc + 2)
    movl $0,         (gdt_desc + 6)

    cli
    lgdt gdt_desc

    ljmp $0x18, $reload_seg
reload_seg:
    mov $0x10, %ax /*data segment selector*/
    mov %ax, %ds
    mov %ax, %ss
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    /* --Paging-- */

    //On active le PAE et le PGE
    movl %cr4, %eax
    orl  $((1<<CR4_PAE) | (1<<CR4_PGE)), %eax
    movl %eax, %cr4

    //On place l'adresse du PML4 dans CR3
    movl $page_ml4, %eax
    movl %eax, %cr3

    //On active le long-mode
    movl  $MSR_EFER, %ecx
    rdmsr
    btsl  $MSR_EFER_LME, %eax
    wrmsr

    //On active le paging
    movl %cr0, %eax
	//+ protection des pages read only
    orl  $((1<<CR0_PG) | (1<<CR0_WP)), %eax
    movl %eax, %cr0

    /*
    Long-mode (sous-mode de compatibilité 32 bits)
    Paging activé
    */

    lgdt gdt_desc
    ljmp $0x18, $reload_seg2

reload_seg2:
    mov $0x10, %ax /*data segment selector*/
    mov %ax, %ds
    mov %ax, %ss
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    movl  $KPA, (bgn_kernel)
    movl  (mb_info), %edi
    mov   (kernel_entry_addr), %eax
    pushl $0x8 //CS
    pushl %eax //IP
    retf       //Far ret

.size _start, . - _start
