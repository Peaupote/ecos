#define ASM_FILE
#include <kernel/sys.h>
	
	.section .data
text0:
    .string "ARGS: argc=0\n"
text1:
    .string "arg:'"
text2:
    .string "'\n"
text3:
    .string "env:'"
argc:
	.quad 0
argv:
	.quad 0
envv:
	.quad 0

    .section .text
.global _start
_start:
	movq    %rdi,  (argc)
	movq    %rsi,  (argv)
	movq    %rdx,  (envv)

	addb    %dil,   (text0 + 11)
    movq    $1,     %rdi
    movq    $text0, %rsi
    movq    $13,    %rdx
    movq    $SYS_WRITE, %rax
    int     $0x80

	movq    (argv), %r15
	movq    (argc), %r14
	movq    $0,     %r13
	jmp     loop_test
loop_begin:
    movq    $1,     %rdi
    movq    $text1, %rsi
    movq    $5,    %rdx
    movq    $SYS_WRITE, %rax
    int     $0x80
   
	movq    (%r15),%rdi
	call    strlen
	movq    %rax,  %rdx
	movq    $1,    %rdi
    movq    (%r15),%rsi
    movq    $SYS_WRITE, %rax
    int     $0x80

	movq    $1,     %rdi
    movq    $text2, %rsi
    movq    $2,     %rdx
    movq    $SYS_WRITE, %rax
    int     $0x80

	leaq    8(%r15), %r15
	incq    %r13
loop_test:
	cmpq    %r13, %r14
	jg		loop_begin
	
	movq    (envv), %r15
	jmp     loop1_test
loop1_begin:
    movq    $1,     %rdi
    movq    $text3, %rsi
    movq    $5,    %rdx
    movq    $SYS_WRITE, %rax
    int     $0x80
   
	movq    (%r15),%rdi
	call    strlen
	movq    %rax,  %rdx
	movq    $1,    %rdi
    movq    (%r15),%rsi
    movq    $SYS_WRITE, %rax
    int     $0x80

	movq    $1,     %rdi
    movq    $text2, %rsi
    movq    $2,     %rdx
    movq    $SYS_WRITE, %rax
    int     $0x80

	leaq    8(%r15), %r15
loop1_test:
	movq    (%r15),%rdi
	testq   %rdi, %rdi
	jne		loop1_begin


    movq    $0, %rdi
    movq    $SYS_EXIT, %rax
    int     $0x80

strlen:
	movq $0, %rax
	jmp loop2_test
loop2_begin:
	incq %rax
	leaq 1(%rdi), %rdi
loop2_test:
	movb  (%rdi), %r8b
	testb  %r8b,  %r8b
	jne   loop2_begin
	ret
