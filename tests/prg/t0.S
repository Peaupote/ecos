    .section .text
.global _start
_start:
    jmp  _start
    movq $0x42, %rax
    movq %rsp, %rdi
    int  $0x80
