#!/usr/bin/awk -f

BEGIN {
	printf("    .quad irq0");
	for (i=1; i<32; i++)
		printf(",irq%d", i);
	printf("\n");
}
{
	printf("irq%d:\n\
    movq $%d, %rdi\n\
    call common_hdl\n\
    add $8, %rsp\n\
    iretq\n", $0,$0);
}
