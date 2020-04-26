#include <libc/stdlib.h>

// syscall
extern void _libc_exit(int);

__attribute__((noreturn))
void exit(int status) {
    _libc_exit(status);
    while(1);
}
