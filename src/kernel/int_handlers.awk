#!/usr/bin/awk -f

/^n /{#none num pile_pop
    printf("irq%d:\n", $2);
    printf("    cli\n");
	if( $3 != 0 ) {
		printf("    movq (%rsp), %rsi\n");
	    printf("    leaq %d(%rsp), %rsp\n", $3);
	}

    printf("    save_c_reg\n");
    printf("    leaq -8(%rsp), %rsp\n");
	printf("	movq $%d, %rdi\n", $2);
    printf("    call common_hdl\n");
    printf("    leaq 8(%rsp), %rsp\n");
    printf("    restore_c_reg\n");
    printf("    sti\n");
    printf("    iretq\n");
}
