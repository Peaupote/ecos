#define ASM_FILE
#include <kernel/sys.h>

    .section .data
fname:
    .string "/proc/1/stat"
string:
    .skip 256
text:
    .string "Hello world !\n"
env0:
	.string "all=42"
exec_fname:
	.string "args.out"
args:
	.quad exec_fname
	.quad text
	.quad 0
envs:
	.quad env0
	.quad 0

    .section .text
    .global _start

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
//    movq    %r15, %rdi  // fd
//    movq    $text, %rsi // src
//    movq    $14, %rdx   // len
//    movq    $SYS_WRITE, %rax
//    int     $0x80

    // go back at beginning of file
//    movq    %r15, %rdi // fd
//    movq    $0, %rsi   // offset
//    movq    $SYS_LSEEK, %rax
//    int     $0x80

    // read file into string
    movq    %r15, %rdi    // fd
    movq    $string, %rsi // dst
    movq    $14, %rdx     // len
    movq    $SYS_READ, %rax
    int     $0x80

    movq    (string), %rax
    int     $0x80

    // print on stdout
    movq    $1, %rdi  // fd
    movq    $string, %rsi // src
    movq    $14, %rdx   // len
    movq    $SYS_WRITE, %rax
    int     $0x80

    movq    $SYS_FORK, %rax
    int     $0x80
    testq   %rax, %rax
    jne     2f

	movq    $exec_fname, %rdi //Non utilisé pour l'instant
	movq    $args,       %rsi
	movq    $envs,       %rdx
	movq    $SYS_EXECVE, %rax
	int     $0x80

    //#PF
//    movq	$42, %rax
//    movq	$0, (%rax)

    //Exit
/*    movq    $0, %rdi
    movq    $SYS_EXIT, %rax
    int     $0x80*/

2:  jmp     2b

1:
    // init wait for all its childrens
    movq    $SYS_WAIT, %rax
    int     $0x80

    jmp     1b
