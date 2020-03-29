#include "tutil.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void* sbrk(intptr_t increment);

void  malloc_init();
void* malloc(size_t dsize);
void  free(void* ptr);
#include "../../src/libc/stdlib/malloc.c"

#define TEST_BUFFER_SIZE 0x2000
#define TEST_NB_ALLOC    50
#define TEST_ALLOC_SIZE  100
#define TEST_REP         100

uint8_t buffer[TEST_BUFFER_SIZE];
bool    buffer_use[TEST_BUFFER_SIZE];
size_t  buf_alloc;
size_t  buf_allocd;

void* sbrk(intptr_t increment) {
	kAssert(increment + (intptr_t)buf_alloc >= 0);
	kAssert(increment + (intptr_t)buf_alloc <= TEST_BUFFER_SIZE);
	void* old  = buffer + buf_alloc;
	for (intptr_t i = 0; i < increment; ++i)
		buffer_use[buf_alloc + i] = false;
	for (intptr_t i = 0; i > increment; --i)
		kAssert(!buffer_use[buf_alloc + i]);
	buf_alloc += increment;
	return old;
}

void reg_alloc(void* addr, size_t sz) {
	kAssert((uint8_t*)addr > buffer);
	kAssert((uint8_t*)addr < buffer + buf_alloc);
	size_t ofs = ((uint8_t*)addr) - buffer;
	for (size_t i = 0; i < sz; ++i) {
		kAssert(!buffer_use[i + ofs]);
		buffer_use[i + ofs] = true;
	}
	buf_allocd += sz;
}
void reg_free(void* addr, size_t sz) {
	size_t ofs = ((uint8_t*)addr) - buffer;
	for (size_t i = 0; i < sz; ++i)
		buffer_use[i + ofs] = false;
	buf_allocd -= sz;
}

void sq_test_malloc() {
	buf_alloc = buf_allocd = 0;
	malloc_init();

	struct {
		void* addr;
		size_t sz;
	} allocs[TEST_NB_ALLOC];
	size_t use0 = buf_alloc;

	for (size_t it = 0; it < TEST_REP; ++it) {

		for (size_t i = 0; i < TEST_NB_ALLOC; ++i) {
			allocs[i].sz   = rand_rng(0, TEST_ALLOC_SIZE);
			allocs[i].addr = TESTED(malloc)(allocs[i].sz);
			reg_alloc(allocs[i].addr, allocs[i].sz);
		}

		size_t free_ord[TEST_NB_ALLOC];
		rand_perm(TEST_NB_ALLOC, free_ord);
		for (size_t i = 0; i < TEST_NB_ALLOC; ++i) {
			size_t j = free_ord[i];
			reg_free(allocs[j].addr, allocs[j].sz);
			TESTED(free)(allocs[j].addr);
		}

		kAssert(buf_alloc == use0);
	}
}

int main() {
	test_init("malloc");
	sq_test_malloc();
	return 0;
}
