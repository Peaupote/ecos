    .section .data
.global keyboard_azerty_layout
.global keyboard_qwerty_layout

#include "keyboard_layout.s"

.global libc_celf
.global root_partition
.global root_partition_end
.global home_partition
.global home_partition_end

.align 0x20
libc_celf:
	.incbin "../libc/libc.out"

.align 0x20
root_partition:
    .incbin "../sys/root.img"
root_partition_end:
.align 0x20
home_partition:
    .incbin "../../tests/run/home.img"
home_partition_end:
.align 0x20

    .section .rodata
    .global font_data

.align 0x20
font_data:
    .incbin "../../data/font.data" // ubuntu mono 9 x 18
.align 0x20
