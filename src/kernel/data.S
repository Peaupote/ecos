    .section .data
.global keyboard_azerty_layout
.global keyboard_qwerty_layout

#include "keyboard_layout.s"

    .section .rodata
    .global t0_data
    .global proc_init

.align 0x20
t0_data:
    .incbin "../../tests/prg/t0.out"
.align 0x20
proc_init:
    .incbin "./aux_prg/init.out"
.align 0x20
