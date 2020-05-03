#define ASM_FILE
#include <headers/sys.h>
.set cst_rax, 0
.set cst_rbx, 1
.set cst_rcx, 2
.set cst_rdx, 3
.set cst_rsi, 4
.set cst_rdi, 5
.set cst_rbp, 6
.set cst_rsp, 7
.set cst_r8,  8
.set cst_r9,  9
.set cst_r10, 10
.set cst_r11, 11
.set cst_r12, 12
.set cst_r13, 13
.set cst_r14, 14
.set cst_r15, 15

.macro ini rg
	movq $cst_\rg , %\rg
.endm
.macro check rg
	cmpq $cst_\rg, %\rg
	jne fail
.endm

	.section .text
	.global _start
_start:
	ini rax
	ini rbx
	ini rcx
	ini rdx
	ini rsi
	ini rdi
	ini rbp
	ini rsp
	ini r8
	ini r9
	ini r10
	ini r11
	ini r12
	ini r13
	ini r14
	ini r15
test:
	check rax
	check rbx
	check rcx
	check rdx
	check rsi
	check rdi
	check rbp
	check rsp
	check r8
	check r9
	check r10
	check r11
	check r12
	check r13
	check r14
	check r15
	jmp test

fail:
	movq $2, %rdi
	movq $fail_msg, %rsi
	movq $6, %rdx
	movq $SYS_WRITE, %rax
	int $0x80

	movq $1, %rdi
	movq $SYS_EXIT, %rax
	int $0x80

	.data
fail_msg:
	.string "error\n"
