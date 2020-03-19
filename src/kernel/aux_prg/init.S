#define ASM_FILE
#include "../sys.h"

    .section .data
fname:
    .string "/home/user/randomfile"
string:
    .skip 256
text:
    .string "Hello world !\n"
textv:
	.string "t0\n"

    .section .text
    .global _start


.macro print_textv
    movq    $1, %rdi     // fd
    movq    $textv, %rsi // src
    movq    $3, %rdx     // len
    movq    $SYS_WRITE, %rax
    int     $0x80
.endm
_start:
    movq    $SYS_FORK, %rax
    int     $0x80

    cmp     $0, %rax
    jne     1f

    movq    $fname, %rdi
    movq    $3, %rsi // chann mode WRITE
    movq    $SYS_OPEN, %rax
    int     $0x80

    // put file descriptor in safe place
    movq    %rax, %r15

    // write hello world onto the file
    movq    %r15, %rdi  // fd
    movq    $text, %rsi // src
    movq    $14, %rdx   // len
    movq    $SYS_WRITE, %rax
    int     $0x80

    // go back at beginning of file
    movq    %r15, %rdi // fd
    movq    $0, %rsi   // offset
    movq    $SYS_LSEEK, %rax
    int     $0x80

    // read file into string
    movq    %r15, %rdi    // fd
    movq    $string, %rsi // dst
    movq    $14, %rdx     // len
    movq    $SYS_READ, %rax
    int     $0x80

    // print on stdout
    movq    $1, %rdi  // fd
    movq    $string, %rsi // src
    movq    $14, %rdx   // len
    movq    $SYS_WRITE, %rax
    int     $0x80

	print_textv
	//Fork
    movq    $SYS_FORK, %rax
    int     $0x80
    testq   %rax, %rax
    jne     2f

	print_textv
	movb	$'2', (1 + textv)
	print_textv

	//Accès au stack
	pushq   $42
	popq    %rax

	//Exit
    movq    $0, %rdi
    movq    $SYS_EXIT, %rax
    int     $0x80
	
2:  
	print_textv
	movb	$'1', (1 + textv)
	print_textv

3:	jmp     3b

1:
    // init wait for all its childrens
    movq    $SYS_WAIT, %rax
    int     $0x80

    jmp     1b
