OUTPUT_FORMAT(elf64-x86-64)

ENTRY(kernel_entry)

INCLUDE ../boot/boot_data.ld

/* adresse virtuelle du kernel,
 * oblige à utiliser des références plus grandes:
 * flag -mcmodel=large de gcc*/
KVA = 0x80 << 39;

kernel_stack_top = KVA + kba_stk_tp;
gd_table   = KVA + kba_gdt;
gdt_desc   = KVA + kba_gdtd;
bgn_kernel = KVA + kba_bgn_krn;
end_kernel = KVA + kba_end_krn;

sptr_alct  = KVA + (0x182 << 30);

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

    kernel_size = . - KVA;
    ASSERT(kernel_size <= 3M, "taille du kernel")
}

/* vérification suplémentaire par tools/check_elf.sh */
kernel_uplim = KVA + 3M;
