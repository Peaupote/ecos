// auto-generated file containing syscall interruptions
#define ASM_FILE
#include<headers/sys.h>
#include<kernel/int.h>
.section .text

.macro mksc name sid
	.global \name
\name :
	movq   $\sid , %rax
	int    $SYSCALL_VEC
	retq
.endm
.macro mksc1 name sid
	.global \name
\name :
	movq   $\sid , %rax
	int    $SYSCALL_R1_VEC
	retq
.endm

mksc sleep        SYS_SLEEP
mksc fork         SYS_FORK
mksc exit         SYS_EXIT
mksc wait         SYS_WAIT
mksc waitpid  	  SYS_WAITPID
mksc getpid       SYS_GETPID
mksc getppid      SYS_GETPPID
mksc open         SYS_OPEN
mksc close        SYS_CLOSE
mksc dup          SYS_DUP
mksc pipe         SYS_PIPE
mksc write        SYS_WRITE
mksc read         SYS_READ
mksc lseek        SYS_LSEEK
mksc execve       SYS_EXECVE
mksc _setpriority SYS_SETPRIO
mksc _getpriority SYS_GETPRIO
mksc debug_block  SYS_DEBUG_BLOCK

mksc1 r1_prires  SYS_R1_PRIRES
