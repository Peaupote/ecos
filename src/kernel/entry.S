.section .text
.global kernel_entry

kernel_entry:
	mov %esp, %eax
	xorq %rsp, %rsp
	mov %eax, %esp
	call kernel_main

	/*infinite loop*/
	cli
1:  hlt
	jmp 1b
