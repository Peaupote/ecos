#define ASSEMBLY
#include "../sys.h"

    .section .text
.global _start
_start:
//    movq    $SYS_FORK, %rax
//    int     $0x80
//
//    cmp     $0, %rax
//    jne     1f
//
//    movq    $SYS_FORK, %rax
//    int     $0x80
//
//    movq    $SYS_GETPID, %rax
//    int     $0x80
//
//    movq    $SYS_GETPPID, %rax
//    int     $0x80
//
//    movq    $42, %rax
//    int     $0x80
//
//    movq    $0, %rdi
//    movq    $SYS_EXIT, %rax
//    int     $0x80
//
1:
    movq    $1, %rdi
    movq    $SYS_SLEEP, %rax
    int     $0x80

    jmp     1b
