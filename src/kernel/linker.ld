/*TODO: taille de l'image générée = 1MB ?!*/
OUTPUT_FORMAT(elf64-x86-64)

ENTRY(kernel_entry)

INCLUDE ../boot/boot_data.ld

/* adresse virtuelle du kernel, 
 * oblige à utiliser des références plus grandes:
 * flag -mcmodel=large de gcc*/
KVA = 0x80 << 39;

stack_top  = KVA + kba_stk_tp;
gd_table   = KVA + kba_gdt;
gdt_desc   = KVA + kba_gdtd;
end_kernel = KVA + kba_end_krn;

low_addr     = KVA + (0x100 << 30);
dynamic_slot = KVA + (0x180 << 30);

SECTIONS
{
    . = KVA + kba_end;
    .text BLOCK(4K) : ALIGN(4K)
    {
		entry.o(.text)
        *(.text)
    }

    /* Read-only data. */
    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    /* Read-write data (initialized) */
    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    /* Read-write data (uninitialized) */
    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }
}
