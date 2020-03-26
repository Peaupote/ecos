	.section .text
	.global proc_idle_entry

proc_idle_entry:// Ring 0, pas de pile
	sti
	hlt
	jmp proc_idle_entry
