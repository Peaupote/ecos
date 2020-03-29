OUTPUT_FORMAT(elf32-i386)

ENTRY(_start)

INCLUDE boot_data.ld

SECTIONS
{
    . = 1M;

    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot) /* Doit être vers le début de l'image*/
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

    /* Read-write data (uninitialized) and stack */
    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
		*(space_for_kernel)
    }
	ASSERT(. <= 6M, "sortie du paging")
}

page_ml4   = KPA + kba_pml4;
page_dpt_1 = KPA + kba_pdpt_1;
page_d_1   = KPA + kba_pd_1;
page_t_23  = KPA + kba_pt_23;
gd_table   = KPA + kba_gdt;
gdt_desc   = KPA + kba_gdtd;
stack_top  = KPA + kba_stk_tp;
end_kernel = KPA + kba_end_krn;
bgn_kernel = KPA + kba_bgn_krn;

