    .section .data
.global keyboard_azerty_layout
.global keyboard_qwerty_layout

#include "keyboard_layout.s"

    .section .rodata
    .global proc_init
    .global proc_sleep

.align 0x20
proc_init:
    .incbin "./aux_prg/init.out"
.align 0x20
proc_sleep:
    .incbin "./aux_prg/loop_sleep.out"
.align 0x20
