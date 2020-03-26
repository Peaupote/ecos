#define ASM_FILE
#include <kernel/sys.h>
#include <kernel/int.h>

    .section .data
pid_cp1:
	.long 0
fname:
    .string "/proc/1/stat"
string:
    .skip 256
text:
    .string "Hello world !\n"
execve_fail:
	.string "execve FAIL\n"
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

_start:// Ring 1
    movq    $SYS_FORK, %rax
    int     $0x80

    cmp     $0, %rax
    jne     1f

	// passage au ring 3
	movq    $3,             %rdi
	movq    $SYS_R1_PRIRES, %rax
	int     $SYSCALL_R1_VEC

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

//    movq    (string), %rax
//    int     $0x80

    // print on stdout
    movq    $1, %rdi  // fd
    movq    $string, %rsi // src
    movq    $14, %rdx   // len
    movq    $SYS_WRITE, %rax
    int     $0x80

3:  
//	jmp     3b
	movq    $0, %rdi
	movq    $SYS_EXIT, %rax
	int     $0x80

1:
	movq    %rax, (pid_cp1)
    movq    $SYS_FORK, %rax
    int     $0x80
    testq   %rax, %rax
    jne     2f
	
	// passage au ring 3
	movq    $3,             %rdi
	movq    $SYS_R1_PRIRES, %rax
	int     $SYSCALL_R1_VEC

	movq    $exec_fname, %rdi //Non utilisé pour l'instant
	movq    $args,       %rsi
	movq    $envs,       %rdx
	movq    $SYS_EXECVE, %rax
	int     $0x80

    movq    $1,    %rdi
    movq    $execve_fail, %rsi
    movq    $12, %rdx
    movq    $SYS_WRITE, %rax
    int     $0x80

	movq    $-1, %rdi
	movq    $SYS_EXIT, %rax
	int     $0x80

2:
	movq    (pid_cp1),    %rdi
	movq    $0,           %rsi
    movq    $SYS_WAITPID, %rax
    int     $0x80
4:
	// init wait for all its childrens
	movq    $0,           %rsi
    movq    $SYS_WAIT,    %rax
    int     $0x80

    jmp     4b
