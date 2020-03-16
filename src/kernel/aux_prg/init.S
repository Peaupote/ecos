#define ASM_FILE
#include "../sys.h"

    .section .data
fname:
    .string "/tmp/file"
string:
    .skip 256
text:
    .string "Hello world !\n"

    .section .text
    .global _start

_start:
    movq    $SYS_FORK, %rax
    int     $0x80

    cmp     $0, %rax
    jne     1f

   movq    $fname, %rdi
   movq    $3, %rsi // chann mode WRITE
   movq    $SYS_OPEN, %rax
   int     $0x80

    // put file descriptor in safe place
    movq    %rax, %r15

    // write hello world onto the file
    movq    %r15, %rdi  // fd
    movq    $text, %rsi // src
    movq    $14, %rdx   // len
    movq    $SYS_WRITE, %rax
    int     $0x80

    // go back at beginning of file
    movq    %r15, %rdi // fd
    movq    $0, %rsi   // offset
    movq    $SYS_LSEEK, %rax
    int     $0x80

    // read file into string
    movq    %r15, %rdi    // fd
    movq    $string, %rsi // dst
    movq    $14, %rdx     // len
    movq    $SYS_READ, %rax
    int     $0x80

    // print on stdout
    movq    $1, %rdi  // fd
    movq    $string, %rsi // src
    movq    $14, %rdx   // len
    movq    $SYS_WRITE, %rax
    int     $0x80

2:  jmp     2b

    movq    $0, %rdi
    movq    $SYS_EXIT, %rax
    int     $0x80

1:
    // init wait for all its childrens
    movq    $SYS_WAIT, %rax
    int     $0x80

    jmp     1b
