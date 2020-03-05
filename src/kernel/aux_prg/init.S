#define ASSEMBLY
#include "../sys.h"

    .section .text
.global _start
_start:
    movq    $SYS_FORK, %rax
    int     $0x80

1:  jmp     1b
