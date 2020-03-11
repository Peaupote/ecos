#define ASM_FILE
#include "../sys.h"

    .section .text
.global _start
_start:
    movq    $SYS_FORK, %rax
    int     $0x80

    cmp     $0, %rax
    jne     1f

    // print return value from fork
    movq    $1, %rdi
    movq    %rax, %rsi
    movq    $SYS_WRITE, %rax
    int     $0x80
3:  jmp     3b


//    movq    $0, %rdi
//    movq    $SYS_EXIT, %rax
//    int     $0x80

1:
//    // print return value from fork
    movq    $1, %rdi
    movq    %rax, %rsi
    movq    $SYS_WRITE, %rax
    int     $0x80

//    movq    $1, %rdi
//    movq    $SYS_SLEEP, %rax
//    int     $0x80

2:
    // init wait for all its childrens
    movq    $SYS_WAIT, %rax
    int     $0x80

    jmp     2b
