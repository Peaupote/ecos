OUTPUT_FORMAT(elf64-x86-64)

ENTRY(null)

null = 0;
libc_ro  = 0;
libc_bss = 0x100000;
libc_end = 0x200000;

SECTIONS
{
    /* Read-only data and code */
	. = libc_ro;
	.note : {*(.note*) }
	. = MAX(., 0x100);
    .text : { *(.text) }
    .rodata : { *(.rodata) }
	ASSERT(. <= libc_bss, "libc data")

    /* Uninitialized data*/
	. = libc_bss;
    .bss :
    {
        *(COMMON)
        *(.bss)
    }

	ASSERT(. <= libc_end, "libc end")
}
