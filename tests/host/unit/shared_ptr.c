#include "tutil.h"

#include <util/paging.h>

uint8_t kmem_paging_alloc(uint_ptr none0 __attribute__((unused)),
			uint16_t none1 __attribute__((unused)),
			uint16_t none2 __attribute__((unused))) {
	return 0;
}
void kmem_paging_free(uint_ptr none __attribute__((unused))){}

#include <kernel/memory/shared_ptr.h>

struct {
	union SPTRAllocator sptr;
	uint8_t vect[0x1000];
} data;
#define sptr_alct data.sptr
#include <src/kernel/memory/shared_ptr.c>

void dump_tree_rec(uint64_t i, size_t ind) {
	test_infoi("%llx\n", i);
	if(spn_at(i)->left) {
		for(size_t j=0; j<ind; ++j)
			test_infoi(". ",0);
		test_infoi("l:",0);
		dump_tree_rec(spn_at(i)->left, ind+1);
	}
	if(spn_at(i)->right) {
		for(size_t j=0; j<ind; ++j)
			test_infoi(". ",0);
		test_infoi("r:",0);
		dump_tree_rec(spn_at(i)->right, ind+1);
	}
}
void dump_tree() {
	if(!data.sptr.root)
		test_infoi("empty\n",0);
	else
		dump_tree_rec(data.sptr.root, 0);
}

void sq_test_tree() {
	sptra_init();
	uint8_t pres[100] = {0};
	uint64_t* ref;
	for (size_t it = 0; it < 1000; ++it) {
		uint8_t rnd_t = rand_rng(0, 3);
		uint8_t idx   = rand_rng(1, 99);
		if (rnd_t == 0) {
			if (pres[idx]) {
				sptra_rem_tree(idx);
				kassert(!sptra_find_ref(idx, &ref), "after rem");
				pres[idx] = 0;
			} else {
				sptra_add_tree(idx);
				kassert(sptra_find_ref(idx, &ref), "after add");
				pres[idx] = 1;
			}
			kassert(sptra_check_tree(), "check tree");
		} else if (rnd_t == 1){
			uint8_t st = sptra_find_ref(idx, &ref);
			kassert((st!=0) == pres[idx], "find");
			if (st) kassert(*ref == idx, "find val");
		} else {
			uint64_t* rtm = sptra_rightmost(&data.sptr.root);
			if (*rtm) {
				kassert(pres[*rtm], "rtm pres");
				for (size_t i=*rtm + 1; i<100; ++i)
					kassert(!pres[i], "rtm sup");
			} else
				for (size_t i=1; i<100; ++i)
					kassert(!pres[i], "rtm empty");
		}
	}
}

void sq_test_alloc() {
	sptra_init();
	bool tk[100] = {0};
	size_t alct = 0;
	for (size_t it = 0; it < 1000; ++it) {
		uint64_t idx = rand_rng(1, 99);
		if (tk[idx]) {
			sptr_free(idx);
			tk[idx] = false;
			--alct;
		} else {
			idx = sptr_alloc();
			kAssert(1 <= idx && idx < 100 && !tk[idx]);
			tk[idx] = true;
			++alct;
		}
		kAssert(sptr_nb_alloc() == alct);
	}
	for (size_t idx = 1; idx < 100; ++idx)
		if (tk[idx]) {
			sptr_free(idx);
			tk[idx] = false;
		}
	kAssert(data.sptr.size == 0);
	kAssert(sptr_nb_alloc() == 0);
	for (size_t i = 1; i < 100; ++i) {
		uint64_t idx = sptr_alloc();
		kAssert(1 <= idx && idx < 100 && !tk[idx]);
		tk[idx] = true;
	}
	kAssert(data.sptr.size == 99);
	kAssert(sptr_nb_alloc() == 99);
	for (size_t i = 2; i < 100; i+=2)
		sptr_free(i);
	kAssert(data.sptr.size == 99);
	kAssert(sptr_nb_alloc() == 50);
	for (size_t i = 99; i > 1; i-=2) {
		sptr_free(i);
		kAssert(data.sptr.size == i - 2);
	}
	sptr_free(1);
	kAssert(data.sptr.size == 0);
	kAssert(sptr_nb_alloc() == 0);
}

int main() {
	test_init("shared_pages");
	sq_test_tree();
	sq_test_alloc();
	return 0;
}
