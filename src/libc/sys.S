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
.macro mksc1j name sid
    .global \name
\name :
    movq   $\sid , %rax
    int    $SYSCALL_R1_VEC
    retq
.endm
.macro mksc1c name sid
    .global \name
\name :
    movq   $(\sid + 0x100), %rax
    int    $SYSCALL_R1_VEC
    retq
.endm

mksc usleep       SYS_USLEEP
mksc fork         SYS_FORK
mksc _exit        SYS_EXIT
mksc wait         SYS_WAIT
mksc waitpid      SYS_WAITPID
mksc getpid       SYS_GETPID
mksc getppid      SYS_GETPPID
mksc open         SYS_OPEN
mksc close        SYS_CLOSE
mksc dup          SYS_DUP
mksc dup2         SYS_DUP2
mksc pipe         SYS_PIPE
mksc write        SYS_WRITE
mksc read         SYS_READ
mksc lseek        SYS_LSEEK
mksc execve       SYS_EXECVE
mksc fstat        SYS_FSTAT
mksc chdir        SYS_CHDIR
mksc _setpriority SYS_SETPRIO
mksc _getpriority SYS_GETPRIO
mksc sbrk         SYS_SBRK
mksc _sigreturn   SYS_SIGRT
mksc _sigsethnd   SYS_SIGHND
mksc _signal      SYS_SIGNAL
mksc kill         SYS_KILL
mksc _seterrno    SYS_SETERRNO
mksc mkdir        SYS_MKDIR
mksc pause        SYS_PAUSE
mksc link         SYS_LINK
mksc symlink      SYS_SYMLINK
mksc readlink     SYS_READLINK

mksc1j r1_prires  SYS_R1J_PRIRES
mksc1c r1_ttyown  SYS_R1C_TTYOWN
