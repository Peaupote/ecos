#define ASSEMBLY
#include "../sys.h"

    .section .text
.global _start
_start:
    movq    $SYS_FORK, %rax
    int     $0x80

    cmp     $0, %rax
    jne     1f

    movq    $0, %rdi
    movq    $SYS_EXIT, %rax
    int     $0x80

1:  jmp     1b
