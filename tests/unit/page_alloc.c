#include "tutil.h"

#include "../../src/kernel/memory/page_alloc.c"

void test_find_8(uint64_t v, uint8_t sz) {
	uint8_t rt = find_in_8(v, sz);
	if(v && !((v>>(sz*rt)) & ((1<<sz)-1))) texit("find 8");
}

struct MemBlock mb;
uint16_t exp_free = 512;

void mb_check_nb() {
	uint16_t f_2_0 = 0;
	for (uint8_t i = 0; i < 8; ++i)
		f_2_0 += (mb.lvl_2_0 >> (i*8)) & 0xff;
	if (f_2_0 != mb.nb_at_lvl[0])
		texit("mb check sum l_2_0");
	if (mb.nb_at_lvl[0] + mb.nb_at_lvl[1]*8 + mb.nb_at_lvl[2]*64
			+ mb.nb_at_lvl[3]*512 != exp_free)
		texit("mb check sum");
}

int main() {
	test_init("page_alloc");
	test_find_8(1L<<47, 8);
	test_find_8(1L<<12, 4);
	test_find_8(1L<<2,  1);
	test_find_8(rand64(), 8);
	test_find_8((uint32_t)rand64(), 4);
	test_find_8((uint8_t)rand64(),  1);

	mblock_init(&mb);
	mb_check_nb();
	uint16_t addr1 = mblock_alloc_page(&mb);
	--exp_free;
	mb_check_nb();
	uint16_t addr2 = mblock_alloc_page(&mb);
	--exp_free;
	mb_check_nb();
	if(addr1 == addr2) texit("collision (1)");
	mblock_free_lvl_0(&mb, addr1);
	mblock_free_lvl_0(&mb, addr2);
	exp_free += 2;
	mb_check_nb();

	uint16_t addrs[512];
	uint8_t  taken[512] = {0};
	for(int c=0; c<=10; ++c){
		uint16_t limbt = rand_rng(0, exp_free);
		uint16_t limtp = rand_rng(limbt, 512);
		if(c == 10) {
			limbt = 0;
			limtp = 512;
		}
		while (exp_free > limbt) {
			--exp_free;
			uint16_t rt = addrs[exp_free] = mblock_alloc_page(&mb);
			if (rt >= 512 || taken[rt]++)
				texit("collision (2)");
			mb_check_nb();
		}
		while(exp_free < limtp) {
			mblock_free_lvl_0(&mb, addrs[exp_free]);
			taken[addrs[exp_free]] = 0;
			++exp_free;
			mb_check_nb();
		}
	}

	return 0;
}
