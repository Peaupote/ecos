#!/usr/bin/awk -f

/^n /{#none num pile_pop
    printf("irq%d:\n", $2);
    printf("    cli\n");
    printf("    save_c_reg\n");
    printf("    leaq -8(%rsp), %rsp\n");
    printf("    call common_hdl\n");
    printf("    leaq 8(%rsp), %rsp\n");
    printf("    restore_c_reg\n");
    printf("    sti\n");
    printf("    iretq\n");
}
