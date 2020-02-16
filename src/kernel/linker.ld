/*TODO: taille de l'image générée = 1MB ?!*/
OUTPUT_FORMAT(elf64-x86-64)

ENTRY(kernel_entry)

SECTIONS
{
    . = 1M;
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
