    .section .data
.global keyboard_azerty_layout
.global keyboard_qwerty_layout

#include "keyboard_layout.s"

    .section .rodata
    .global proc_init
    .global edummy_args
    .global home_partition

.align 0x20
proc_init:
    .incbin "./aux_prg/init.out"
.align 0x20
edummy_args:
    .incbin "../../tests/prg/args.out"
.align 0x20
home_partition:
    .incbin "../../tests/ext2/home.img"
.align 0x20
