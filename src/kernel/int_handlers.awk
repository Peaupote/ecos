#!/usr/bin/awk -f

BEGIN {
	printf("    .quad irq0");
	for (i=1; i<32; i++)
		printf(",irq%d", i);
	printf("\n");
}
/^n /{#none num pile_pop
	printf("irq%d:\n", $2);
	if( $3 != 0 )
    printf("    leaq %d(%rsp), %rsp\n", $3);
    printf("    iretq\n");
}
