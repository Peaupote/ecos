#include <libc/stdlib.h>

// syscall
__attribute__((noreturn))
extern void _libc_exit(int);

__attribute__((noreturn))
void exit(int status) {
    _libc_exit(status);
}
