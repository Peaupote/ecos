#include <libc/unistd.h>

#include <stddef.h>

int brk(void* addr) {
	void* cbrk = sbrk(0);
	void* nbrk = sbrk(addr - cbrk);
	return nbrk == addr;
}
