#define ASM_FILE
#include<headers/sys.h>
#include<kernel/int.h>
.section .text

	.global libc_sighandler
	.extern sig_handlers

libc_sighandler:// rsp = 0 [16]
	movq   sig_handlers(, %edi, 8), %rax
	incl   %edi //sigid -> signum
	callq  *%rax

	movq   $SYS_SIGRT, %rax
	int    $SYSCALL_VEC
