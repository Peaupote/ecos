#include "tutil.h"

typedef uint64_t phy_addr;

#include "../../src/kernel/memory/page_alloc.c"

void test_find_8(uint64_t v, uint8_t sz) {
    uint8_t rt = find_bit_64(v, sz, 3);
	if (v)
		kAssert(((v>>(sz*rt))     & ((1<<sz)-1))
			&& (rt == 7 || !((v>>(sz*(rt+1))) & ((1<<sz)-1))));
	else
		kAssert(rt == 0);
}
void sq_test_find_8() {
    test_find_8(1L<<47, 8);
    test_find_8(1L<<12, 4);
    test_find_8(1L<<2,  1);
    test_find_8(rand64(), 8);
    test_find_8((uint32_t)rand64(), 4);
    test_find_8((uint8_t)rand64(),  1);
}

void sq_test_qsort() {
    uint64_t a[100];
    uint64_t cs = 0, cs2 = 0;
    for (size_t i = 0; i < 100; ++i)
        cs += (a[i] = rand64());
    sort_limits(a, 100);
    for (size_t i = 1; i < 100; ++i)
        if (a[i-1] > a[i])
            texit("sort");
    for(size_t i = 0; i < 100; ++i)
        cs2 += a[i];
    if (cs2 != cs)
        texit("sort cs");
}

struct MemBlock mb;
uint16_t exp_free = 512;

void mb_check_nb() {
    uint16_t f_2_0 = 0;
    for (uint8_t i = 0; i < 8; ++i)
        f_2_0 += (mb.lvl_2_0 >> (i*8)) & 0xff;
    if (f_2_0 != mb.nb_at_lvl[0])
        texit("mb check sum l_2_0");
    if (mblock_nb_page_free(&mb) != exp_free)
        texit("mb check sum");
}

void sq_test_mblock() {
    mblock_init1(&mb);
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

}

struct   MemBlockTree mbt;
uint64_t space_mbt[100];
uint8_t  set_elem[512] = {0};
size_t   nb_elem = 0;

void sq_test_mbt() {
    size_t sz    = rand_rng(100, 512);
    size_t h     = mbtree_height_for(sz);
    size_t intn  = mbtree_intn_for(h);
    size_t space = mbtree_space_for(intn, sz);
    if(space > 800) texit("mbt space");
    mbtree_init(&mbt, h, intn, space, space_mbt);
    test_infoi("\tmbt--sz=%d ",     sz);
    test_infoi("h=%d ",      h);
    test_infoi("intn=%d ",   intn);
    test_infoi("space=%d\n",  space);
    for(size_t i=0;i<1000;++i) {
        uint64_t idx = rand_rng(0, sz+50);
        if (idx >= sz) {
            if (nb_elem) {
                if (!mbtree_non_empty(&mbt))
                    texit("false mbt non empty (expect 1)");
                size_t e = mbtree_find(&mbt);
                if (e >= sz || !set_elem[e])
                    texit("echec mbt_find");
            } else if (mbtree_non_empty(&mbt))
                texit("false mbt non empty (expect 0)");
        } else if(rand_rng(0,1)) {
            if (set_elem[idx]) {
                --nb_elem;
                set_elem[idx] = 0;
            }
            mbtree_rem(&mbt, idx);
            if (mbtree_get(&mbt, idx)) texit("mbt_rem");
        } else {
            if (!set_elem[idx]) {
                ++nb_elem;
                set_elem[idx] = 1;
            }
            mbtree_add(&mbt, idx);
            if (!mbtree_get(&mbt, idx)) texit("mbt_add");
        }
    }
}

int main() {
    test_init("page_alloc");
    sq_test_find_8();
    sq_test_qsort();
    sq_test_mblock();
    sq_test_mbt();
    return 0;
}
