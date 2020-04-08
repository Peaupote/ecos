#define ASM_FILE
#include <headers/sys.h>
    .section .text
    .global _start
    .extern _env

_start:
    movq  %rdi, %r12
    movq  %rsi, %r13
    movq  %rdx, %r14
    call  _libc_init
    movq  %r12, %rdi
    movq  %r13, %rsi
    movq  $_env, %rdx
    call  main
    movq  %rax,      %rdi
    call  _libc_exit
