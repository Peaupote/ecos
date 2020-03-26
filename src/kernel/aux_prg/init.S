#define ASM_FILE
#include <kernel/sys.h>

    .section .data
fname1:
    .string "/home/mpouzet/hello-world"
fname2:
    .string "/home/mpouzet/hello-world.s"
fname3:
    .string "/home/mpouzet/bio.txt"
#define BUF_SZ 1024
buf:
    .skip BUF_SZ
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

.macro read_print
    movq    %r15, %rdi
    movq    $buf, %rsi
    movq    $BUF_SZ, %rdx
    movq    $SYS_READ, %rax
    int     $0x80

    movq    $1, %rdi
    movq    $buf, %rsi
    movq    %rax, %rdx
    movq    $SYS_WRITE, %rax
    int     $0x80
.endm

.macro sleep
    movq    $3, %rdi
    movq    $SYS_SLEEP, %rax
    int     $0x80
.endm

_start:
    movq    $SYS_FORK, %rax
    int     $0x80

    cmp     $0, %rax
    jne     1f

    // if I dont fork here the scheduler block on sleep
    movq    $SYS_FORK, %rax
    int     $0x80

    cmp     $0, %rax
    jne     3f

    sleep

    // cat /home/mpouzet/hello-world
    movq    $fname1, %rdi
    movq    $1, %rsi
    movq    $SYS_OPEN, %rax
    int     $0x80


    // save file descriptor some place safe
    movq    %rax, %r15
    read_print

    movq    %r15, %rdi
    movq    $SYS_CLOSE, %rax
    int     $0x80

    sleep

    // cat /home/mpouzet/hello-world.s
    movq    $fname2, %rdi
    movq    $1, %rsi
    movq    $SYS_OPEN, %rax
    int     $0x80

    // save file descriptor some place safe
    movq    %rax, %r15
    read_print

    movq    %r15, %rdi
    movq    $SYS_CLOSE, %rax
    int     $0x80

    sleep

    // cat /home/mpouzet/bio.txt
    movq    $fname3, %rdi
    movq    $1, %rsi
    movq    $SYS_OPEN, %rax
    int     $0x80

    // save file descriptor some place safe
    movq    %rax, %r15
    read_print
    read_print

    movq    %r15, %rdi
    movq    $SYS_CLOSE, %rax
    int     $0x80

3:  jmp     3b

1:
//    movq    $SYS_FORK, %rax
//    int     $0x80
//    testq   %rax, %rax
//    jne     2f
//
//    movq    $exec_fname, %rdi //Non utilisé pour l'instant
//    movq    $args,       %rsi
//    movq    $envs,       %rdx
//    movq    $SYS_EXECVE, %rax
//    int     $0x80

2:
    // init wait for all its childrens
    movq    $SYS_WAIT, %rax
    int     $0x80

    jmp     2b
