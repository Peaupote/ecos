#define ASM_FILE
#include <kernel/sys.h>

    .section data
buf:
    .string "Hello world !"

    .section text
    .global _start

_start:
    movq    $1, %rdi
    movq    $buf, %rsi
    movq    $14, %rdx
    movq    $SYS_WRITE, %rax
