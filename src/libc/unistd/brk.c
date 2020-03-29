#include <libc/unistd.h>

#include <stddef.h>

int brk(void* addr __attribute__((unused)) ) { // TODO
	return -1;
}

void* sbrk(intptr_t increment __attribute__((unused)) ) { //TODO
	return NULL;
}
