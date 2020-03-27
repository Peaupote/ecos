#define ASM_FILE
#include <headers/sys.h>
	.section .text
	.global _start

_start:
	pushq $42
	call  main
	movq  %rax,      %rdi
	movq  $SYS_EXIT, %rax
	int   $0x80
