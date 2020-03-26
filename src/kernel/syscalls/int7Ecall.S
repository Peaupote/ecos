#define ASM_FILE
#include <kernel/int.h>
	.section .text

.macro clwr fun
	.global call_\fun
call_\fun :
	movabs $\fun , %rax
	int    $SYSCALL_R1_CALL_VEC
	retq
.endm

clwr proc_execve_error
clwr proc_execve_error_2
clwr execve_switch_pml4
clwr proc_execve_end
clwr kmem_paging_alloc_rng
clwr execve_tr_do_alloc_pg
