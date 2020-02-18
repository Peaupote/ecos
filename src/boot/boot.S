#include "def.h"

/* From https://wiki.osdev.org/Bare_Bones */
/* Declare constants for the multiboot header. */
.set ALIGN,    1<<0             /* align loaded modules on page boundaries */
.set MEMINFO,  1<<1             /* provide memory map */
.set FLAGS,    ALIGN | MEMINFO  /* this is the Multiboot 'flag' field */
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

	.section .bss

//Paging structures for id mapping
.global page_dpt_0
.global page_d_0
.global page_t_01

.global mb_info
.global kernel_entry_addr

#ifdef BDI
//TODO
.global boot_debug_out
#endif

//Pour les asserts de ld
.global end_used_bss
.global end_space_for_kernel

//Structures pour le mapping identité sur les 4 premiers MB
.balign 0x1000
page_dpt_0: #PDPT
.skip 0x1000 # 512 * 64 bits

.balign 0x1000
page_d_0:
.skip 0x1000 # 512 * 64 bits

.balign 0x1000
page_t_01:
.skip 0x2000 # 2*512 * 64 bits

.balign 4
mb_info:
.skip 4
kernel_entry_addr:
.skip 4

#ifdef BDI
boot_debug_out:
.skip 0x100 # 256 octets de debug
#endif

//espace pour le kernel
//évite que GRUB charge le module sur une adresse utilisée
	.section space_for_kernel
.global KPA //Adresse physique du kernel
.balign 0x1000 //alignement à la page 4KB
KPA: .skip 0x200000 //2MB


/*
The linker script specifies _start as the entry point to the kernel and the
bootloader will jump to this position once the kernel has been loaded. It
doesn't make sense to return from this function as the bootloader is gone.
*/
	.section .text
.global _start
.type _start, @function
_start:

	//On suppose que l'on est démarré par GRUB
	//Mode protégé avec des segments 'plats'
	//Paging désactivé
	//Ligne A20 activée

	//On récupère les informations de multiboot
	movl %ebx, mb_info

#ifdef BDI
	//initial CR0
	mov %cr0, %eax
	mov %eax, boot_debug_out

	//A20 test:
	mov $0x00042, %eax
	movb (%eax), %al
	movb %al, (boot_debug_out+8)
	mov $0x100042, %eax
	movb (%eax), %al
	mov %al, (boot_debug_out+9)
#endif


	//Stack
    mov $stack_top, %esp

	/*
	 On charge le kernel ici afin d'être sûr de pouvoir accéder
	 aux données fournies par GRUB
	 On récupère l'adresse du kernel 64 bits dans kernel_entry_addr
	*/
	call load_main

	/* --GDT-- */
#include "gdt.s"
	/* GDT descriptor */
	movw $(4*64 - 1), gdt_desc
	movl $gd_table,  (gdt_desc + 2)
	movl $0,         (gdt_desc + 6)

	cli
   	lgdt  gdt_desc
	ljmp $0x18, $reload_seg
reload_seg:
	mov $0x10, %ax /*data segment selector*/
	mov %ax, %ds
	mov %ax, %ss
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

#ifdef TEST_PAGING
	call test_paging_pre
#endif
	/* --Paging-- */
	call init_paging_directory

	//On active le PAE - Physical Address Extension
	movl %cr4, %eax
	btsl $5,   %eax
	movl %eax, %cr4

	//On place l'adresse du PML4 dans CR3
	movl $page_ml4, %eax
	movl %eax, %cr3

	//On active le long-mode
	movl $0xC0000080, %ecx //adresse du MSR EFER
	rdmsr
	btsl  $8, %eax
	wrmsr
	
	//On active le paging: bit 31 de CR0
	movl %cr0, %eax
	btsl $31,  %eax
	movl %eax, %cr0

	/*
	Long-mode (sous-mode de compatibilité 32 bits)
	Paging activé
	*/

#ifdef BDI
	movl %cr0, %eax
	movl %eax, (boot_debug_out+4)
#endif

	lgdt gdt_desc
	ljmp $0x18, $reload_seg2

reload_seg2:
	mov $0x10, %ax /*data segment selector*/
	mov %ax, %ds
	mov %ax, %ss
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

   	mov (kernel_entry_addr), %ebx

	ljmp $0x8, $enter_64b
enter_64b:
#include "indj_64.s"
	
/*
Set the size of the _start symbol to the current location '.' minus its start.
This is useful when debugging or when you implement call tracing.
*/
.size _start, . - _start
