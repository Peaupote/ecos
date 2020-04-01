#define ASM_FILE
#include <kernel/sys.h>

    .section .text
    .global proc_init_entry

proc_init_entry:// Ring 1, pas de pile
    movq    $SYS_FORK, %rax
    int     $0x80

    cmp     $0, %rax
    jne     1f

    movabs  $init1_path, %rdi
    movabs  $init1_args, %rsi
    movq    %rsi,        %rdx
    movq    $SYS_EXECVE, %rax
    int     $0x80

2:	jmp 2b //Erreur

1:
    // init wait for all its childrens
    movq    $SYS_WAIT, %rax
    int     $0x80

    jmp     1b

    .section .data
init1_path:
    .string "/home/bin/sh"
init1_args:
    .quad 0
