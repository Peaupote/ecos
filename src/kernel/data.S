    .section .data
.global keyboard_azerty_layout
.global keyboard_qwerty_layout

#include "keyboard_layout.s"

    .section .rodata
    .global home_partition
    .global home_partition_end

.align 0x20
home_partition:
    .incbin "../../tests/run/home.img"
home_partition_end:
.align 0x20
