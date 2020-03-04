#define ASSEMBLY
#include "int.h"
#include "sys.h"

    .section .text
    .global sleep

sleep:
    mov $SYS_SLEEP, %rax
    int $SYSCALL_VEC
    ret
