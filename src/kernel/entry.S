#define ASSEMBLY

    .section .text
.global kernel_entry

.extern kernel_main

kernel_entry:
	//On passe %rip en adresse virtuelle
	movabsq $kernel_entry_vrip, %rbx
	jmp *%rbx
kernel_entry_vrip:

    //On charge la nouvelle adresse virtuelle de la pile
    movabsq $stack_top, %rsp

    call kernel_main

    //infinite loop
    cli
1:  hlt
    jmp 1b
