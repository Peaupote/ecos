#define ASSEMBLY
#include "int.h"
#include "sys.h"

    .section .text
    .global sleep

sleep:
    imul $PIT_FREQ, %rdi
    mov $SLEEP, %rax
    int $SYSCALL_VEC
    ret
