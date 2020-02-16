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
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/*
The multiboot standard does not define the value of the stack pointer register
(esp) and it is up to the kernel to provide a stack. This allocates room for a
small stack by creating a symbol at the bottom of it, then allocating 16384
bytes for it, and finally creating a symbol at the top. The stack grows
downwards on x86. The stack is in its own section so it can be marked nobits,
which means the kernel file is smaller because it does not contain an
uninitialized stack. The stack on x86 must be 16-byte aligned according to the
System V ABI standard and de-facto extensions. The compiler will assume the
stack is properly aligned and failure to align the stack will result in
undefined behavior.
*/
.section .bss
.global page_ml4   #Page Map Level 4
.global page_dpt_0 #Page Directory Pointer Table
.global page_d_0   #Page Directory
.global page_t_01  #Page Tables

.global mb_info
.global kernel_entry_addr
#ifdef BDI
.global boot_debug_out
#endif
.global end_used_bss
.global end_space_for_kernel

.align 16
stack_bottom:
.skip 0x4000 # 16 KiB
stack_top:

//--Paging structures--
.align 0x1000 # 4 KiB - page aligned
page_ml4: #PML4
.skip 0x1000 # 512 * 64 bits

.align 0x1000
page_dpt_0: #PDPT
.skip 0x1000 # 512 * 64 bits

.align 0x1000
page_d_0:
.skip 0x1000 # 512 * 64 bits

.align 0x1000
page_t_01:
.skip 0x2000 # 2*512 * 64 bits

.align 4
mb_info:
.skip 4
kernel_entry_addr:
.skip 4

#ifdef BDI
boot_debug_out:
.skip 0x100 # 256 octets de debug
#endif
end_used_bss:
//space_for_kernel
.skip 0x200000
end_space_for_kernel:

.section .data
.global gd_table

/* Tout les segments couvrent l'ensemble de la mémoire */
gd_table:
.long GDT_ENTRY_0( 0, 0 )
.long GDT_ENTRY_1( 0, 0, 0, 0 )
//code segment 64bits
.long GDT_ENTRY_0( 0, 0xfffff )
.long GDT_ENTRY_1( 0, 0xfffff, 0x9a, 0xa )
//data segment
.long GDT_ENTRY_0( 0, 0xfffff )
.long GDT_ENTRY_1( 0, 0xfffff, 0x92, 0xc )
//code segment
.long GDT_ENTRY_0( 0, 0xfffff )
.long GDT_ENTRY_1( 0, 0xfffff, 0x9a, 0xc )

gdt_descriptor:
.short (4*64 - 1) #taille - 1
gdt_descriptor_ofs0:
.long gd_table  #offset
.long 0			#for long mode


/*
The linker script specifies _start as the entry point to the kernel and the
bootloader will jump to this position once the kernel has been loaded. It
doesn't make sense to return from this function as the bootloader is gone.
*/
.section .text
.global _start
.type _start, @function
_start:

	//On suppose que l'on est démarré pat GRUB
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
	cli
   	lgdt  gdt_descriptor
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
	orl  $(1<<5), %eax
	movl %eax, %cr4

	//On place l'adresse du PML4 dans CR3
	movl $page_ml4, %eax
	movl %eax, %cr3

	//On active le long-mode
	movl $0xC0000080, %ecx //adresse du MSR EFER
	rdmsr
	orl  $(1<<8), %eax
	wrmsr
	
	//On active le paging: bit 31 de CR0
	movl %cr0, %eax
	orl  $(1<<31), %eax
	movl %eax, %cr0

	/*
	Long-mode (sous-mode de compatibilité 32 bits)
	Paging activé
	*/

#ifdef BDI
	movl %cr0, %eax
	movl %eax, (boot_debug_out+4)
#endif

	lgdt gdt_descriptor
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
	//xorq %rax, %rax
	.byte 0x48
	.byte 0x31
	.byte 0xc0
	//mov %ebx, %eax
	.byte 0x89
	.byte 0xd8
	//jmpq *%eax
	.byte 0xff
	.byte 0xe0
	
/*
Set the size of the _start symbol to the current location '.' minus its start.
This is useful when debugging or when you implement call tracing.
*/
.size _start, . - _start
