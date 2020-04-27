#include "../tutil.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

void* sbrk(intptr_t increment);

void  malloc_init();
void* malloc(size_t dsize);
void  free(void* ptr);
#include <src/libc/stdlib/malloc.c>

#define TEST_BUFFER_SIZE 0x2000
#define TEST_NB_ALLOC    50
#define TEST_ALLOC_SIZE  100
#define TEST_REP0        100
#define TEST_REP1        10000

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

static inline bool is_valid_fb(free_bloc_t* p) {
	if (p == &root) return true;
	uint8_t* u = (uint8_t*)p;
	if (u < buffer) return false;
	if (u + sizeof(free_bloc_t) > buffer + buf_alloc) return false;
	size_t ofs = u - buffer;
	for (size_t i = 0; i < sizeof(free_bloc_t); ++i)
		if (buffer_use[ofs]) return false;
	return true;
}
void print_llist() {
    for (free_bloc_t* it = root.next; it != &root; it = it->next) {
        uint32_t* hd = bloc_head(it);
		printf("\t+%ld (%p) : %d - %x", (uint8_t*)it - buffer, it,
				(*hd)&MBLOC_SIZE_MASK, (*hd)&0x7);
	}
}

bool check_llist() {
	if (!is_valid_fb(root.next)) return false;
	if (!is_valid_fb(root.prev)) return false;
    for (free_bloc_t* it = root.next; it != &root; it = it->next) {
		if (!is_valid_fb(it->next)) return false;
		if (!is_valid_fb(it->prev)) return false;
	}
	return true;
}

void reg_alloc(void* addr, size_t sz) {
	if (sz) {
		kAssert((uint8_t*)addr > buffer);
		kAssert(((uint8_t*)addr) + sz <= buffer + buf_alloc);
		size_t ofs = ((uint8_t*)addr) - buffer;
		ofs -= MALLOC_HD_SZ;
		for (size_t i = 0; i < sz + MALLOC_HD_SZ; ++i) {
			tAssert(!buffer_use[i + ofs]);
			buffer_use[i + ofs] = true;
		}
		buf_allocd += sz;
	}
}
void reg_free(void* addr, size_t sz) {
	if (sz) {
		size_t ofs = ((uint8_t*)addr) - buffer;
		ofs -= MALLOC_HD_SZ;
		for (size_t i = 0; i < sz + MALLOC_HD_SZ; ++i) {
			tAssert(buffer_use[i + ofs]);
			buffer_use[i + ofs] = false;
		}
		buf_allocd -= sz;
	}
}

void sq_test_malloc() {
	buf_alloc = buf_allocd = 0;
	_malloc_init();

	struct {
		void* addr;
		size_t sz;
	} allocs[TEST_NB_ALLOC];
	size_t use0 = buf_alloc;

	for (size_t it = 0; it < TEST_REP0; ++it) {

		for (size_t i = 0; i < TEST_NB_ALLOC; ++i) {
			allocs[i].sz   = rand_rng(0, TEST_ALLOC_SIZE);
			allocs[i].addr = TEST_U(malloc)(allocs[i].sz);
			reg_alloc(allocs[i].addr, allocs[i].sz);
		}

		size_t free_ord[TEST_NB_ALLOC];
		rand_perm(TEST_NB_ALLOC, free_ord);
		for (size_t i = 0; i < TEST_NB_ALLOC; ++i) {
			size_t j = free_ord[i];
			reg_free(allocs[j].addr, allocs[j].sz);
			TEST_U(free)(allocs[j].addr);
		}

		kAssert(buf_alloc == use0);
		kAssert(check_llist());
	}

	for (size_t i = 0; i < TEST_NB_ALLOC; ++i)
		allocs[i].sz = ~(size_t)0;
	for (size_t it = 0; it < TEST_REP1; ++it) {
		int j = rand_rng(0, TEST_NB_ALLOC - 1);
		if (~allocs[j].sz) {
			reg_free(allocs[j].addr, allocs[j].sz);
			TEST_U(free)(allocs[j].addr);
			kAssert(check_llist());
			allocs[j].sz = ~(size_t)0;
		} else {
			allocs[j].sz   = rand_rng(0, TEST_ALLOC_SIZE);
			allocs[j].addr = TEST_U(malloc)(allocs[j].sz);
			kAssert(check_llist());
			reg_alloc(allocs[j].addr, allocs[j].sz);
		}
	}
}

int main() {
	test_init("malloc");
	sq_test_malloc();
	return 0;
}
