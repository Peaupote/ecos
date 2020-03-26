#include <kernel/memory/page_alloc.h>

#include <util/misc.h>

#include <util/paging.h>
#ifndef TEST_UNIT
#include <kernel/kutil.h>
#endif

#define QD_0111_MASK 0x77777777
#define QD_1000_MASK 0x88888888

void mblock_init1(struct MemBlock* b) {
    for(uint64_t* it = (uint64_t*)b;
            it != ((uint64_t*)b) + sizeof(struct MemBlock) / 8; ++it)
        *it = 0;
    b->nb_at_lvl[3] = 1;
}

void mblock_init0(struct MemBlock* b) {
    for(uint64_t* it = (uint64_t*)b;
            it != ((uint64_t*)b) + sizeof(struct MemBlock) / 8; ++it)
        *it = 0;
}

static inline void decr_qd(uint32_t* s, uint8_t i) {
    ((uint8_t*)s)[i>>1] = (((uint8_t*)s)[i>>1]) - ((i & 1) ? 0x10 : 0x01);
}
static inline uint8_t incr_qd(uint32_t* s, uint8_t i) {
    if (i&1) {
        uint8_t rt = ((uint8_t*)s)[i>>1] = (((uint8_t*)s)[i>>1]) + 0x10;
        return rt >> 4;
    }
    uint8_t rt = ((uint8_t*)s)[i>>1] = (((uint8_t*)s)[i>>1]) + 0x1;
    return rt & 0xf;
}
static inline void set_qs(uint32_t* s, uint8_t i, uint8_t v) {
    ((uint8_t*)s)[i>>1] = (i&1)
        ? ((((uint8_t*)s)[i>>1]) & 0x0f) | (v<<4)
        : ((((uint8_t*)s)[i>>1]) & 0xf0) |  v;
}

static inline uint8_t bit_1(uint8_t i) {
    return ((uint8_t)1)<<i;
}
static inline uint32_t bit_4(uint8_t i) {
    return ((uint32_t)0x8)<<(4*i);
}
static inline uint32_t val_4(uint8_t i, uint32_t v) {
    return v<<(4*i);
}
static inline uint64_t val_8(uint8_t i, uint64_t v) {
    return v<<(8*i);
}

uint16_t mblock_alloc_lvl_0(struct MemBlock* b) {
    uint16_t n;
    uint8_t  l0, l1, l2;

    --b->nb_at_lvl[0];

    n = l2 = find_bit_64(b->lvl_2_0, 8, 3);
    --(((uint8_t*)&b->lvl_2_0)[l2]);

    l1 = find_bit_32(b->lvl_1_10[n] & QD_0111_MASK, 4, 3);
    decr_qd(b->lvl_1_10 + n, l1);
    n  = (n<<3) | l1;

    l0 = find_bit_8(b->lvl_0_0 [n], 1, 3);
    b->lvl_0_0 [n] &= ~bit_1(l0);
    n  = (n<<3) | l0;

    return n;
}

uint16_t mblock_alloc_lvl_1(struct MemBlock* b) {
    uint16_t n;
    uint8_t  l1, l2;

    --b->nb_at_lvl[1];

    n = l2 = find_bit_32(b->lvl_2_21 & QD_0111_MASK, 4, 3);
    decr_qd(&b->lvl_2_21, l2);

    l1 =  find_bit_32(b->lvl_1_10[n] & QD_1000_MASK, 4, 3);
    b->lvl_1_10 [n] &= ~bit_4(l1);
    n  = (n<<3) | l1;

    return n<<3;
}

uint16_t mblock_alloc_lvl_2(struct MemBlock* b) {
    uint16_t n;
    uint8_t  l2;

    --b->nb_at_lvl[2];

    n = l2 = find_bit_32(b->lvl_2_21 & QD_1000_MASK, 4, 3);
    b->lvl_2_21 &= ~bit_4(l2);

    return n<<6;
}

void mblock_alloc_lvl_3(struct MemBlock* b) {
    --b->nb_at_lvl[3];
}
uint16_t mblock_alloc_lvl_3f(struct MemBlock* b) {
    mblock_free_lvl_3(b);
    return 0;
}

mblock_alloc_f mblock_alloc[4] = {
    mblock_alloc_lvl_0, mblock_alloc_lvl_1,
    mblock_alloc_lvl_2, mblock_alloc_lvl_3f
};


void mblock_free_lvl_3(struct MemBlock* b) {
    ++b->nb_at_lvl[3];
}
void mblock_free_lvl_3f(struct MemBlock* b,
        uint16_t none __attribute__((unused))) {
    mblock_free_lvl_3(b);
}

void mblock_free_lvl_2(struct MemBlock* b, uint16_t n) {
    if (++b->nb_at_lvl[2] == 8) {
        b->lvl_2_21 = 0;
        b->nb_at_lvl[2] = 0;
        b->nb_at_lvl[3] = 1;
    } else {
        uint8_t l2 = n >> 6;
        b->lvl_2_21 |= bit_4(l2);
    }
}

void mblock_free_lvl_1(struct MemBlock* b, uint16_t n) {
    uint8_t l2 = n>>6;
    if (incr_qd(&b->lvl_2_21, l2) & 0x8) {
        b->lvl_1_10[l2] = 0;
        b->nb_at_lvl[1] -= 7;
        mblock_free_lvl_2(b, n);
    } else {
        uint8_t l1 = (n >> 3) & 0x7;
        b->lvl_1_10[l2] |= bit_4(l1);
        ++b->nb_at_lvl[1];
    }
}

void mblock_free_lvl_0(struct MemBlock* b, uint16_t n) {
    uint8_t l2 = n>>6;
    uint8_t l1 = (n >> 3) & 0x7;
    if (incr_qd(b->lvl_1_10 + l2, l1) & 0x8) {
        b->lvl_0_0[n>>3] = 0;
        b->lvl_2_0 -= val_8(l2, 7);
        b->nb_at_lvl[0] -= 7;
        mblock_free_lvl_1(b, n);
    } else {
        uint8_t l0 = n & 0x7;
        b->lvl_0_0[n>>3] |= bit_1(l0);
        b->lvl_2_0 += val_8(l2, 1);
        ++b->nb_at_lvl[0];
    }
}

mblock_free_f mblock_free[4] = {
    mblock_free_lvl_0, mblock_free_lvl_1,
    mblock_free_lvl_2, mblock_free_lvl_3f
};

//keep > 0
//conserve les `keep` premiers sous-blocs
void mblock_split_lvl_1(struct MemBlock* b, uint16_t n, uint8_t keep) {
    uint8_t l2   = n >> 6;
    uint8_t l1   = (n >> 3) & 0x7;
    uint8_t free = 8 - keep;
    b->nb_at_lvl[0] += free;
    b->lvl_2_0      += val_8(l2, free);
    set_qs(b->lvl_1_10 + l2, l1, free);
    b->lvl_0_0[n>>3] = 0xff << keep;
}
void mblock_split_lvl_2(struct MemBlock* b, uint16_t n, uint8_t keep) {
    uint8_t l2   = n >> 6;
    uint8_t free = 8 - keep;
    b->nb_at_lvl[1] += free;
    b->lvl_2_21     += val_4(l2, free);
    b->lvl_1_10[l2]  = ((uint32_t)QD_1000_MASK) << (keep * 4);
}
void mblock_split_lvl_3(struct MemBlock* b, uint8_t keep) {
    uint8_t free = 8 - keep;
    b->nb_at_lvl[2] += free;
    b->lvl_2_21      = ((uint32_t)QD_1000_MASK) << (keep * 4);
}
void mblock_split_lvl_3f(struct MemBlock* b,
        uint16_t n __attribute__((unused)), uint8_t keep) {
    mblock_split_lvl_3(b, keep);
}
mblock_split_f mblock_split[3] = {
    mblock_split_lvl_1, mblock_split_lvl_2, mblock_split_lvl_3f
};


uint16_t mblock_alloc_page(struct MemBlock* b) {
    uint16_t rt;
    if (!b->nb_at_lvl[0]) {
        if (!b->nb_at_lvl[1]) {
            if (!b->nb_at_lvl[2]) {
                rt = 0;
                mblock_alloc_lvl_3(b);
                mblock_split_lvl_3(b,1);
            } else rt = mblock_alloc_lvl_2(b);
            mblock_split_lvl_2(b, rt, 1);
        } else rt = mblock_alloc_lvl_1(b);
        mblock_split_lvl_1(b, rt, 1);
    } else rt = mblock_alloc_lvl_0(b);
    return rt;
}

uint8_t mblock_non_empty(struct MemBlock* b) {
    return (*((uint64_t*)b->nb_at_lvl)) ? 1 : 0;
}
uint8_t mblock_full_free(struct MemBlock* b) {
    return b->nb_at_lvl[3];
}
size_t mblock_nb_page_free(struct MemBlock* b) {
    size_t rt = 0;
    for (uint8_t l = 0; l < 4; ++l)
        rt += b->nb_at_lvl[l] * mblock_page_size_lvl(l);
    return rt;
}

void mblock_free_rng_aux(struct MemBlock* b, uint16_t bg, uint16_t ed,
        uint8_t lvl, uint16_t n) {
    uint16_t m = n + mblock_page_size_lvl(lvl);
    if (bg <= n && ed >= m)
        (*mblock_free[lvl])(b, n);
    else if (bg < m && ed > n)
        for (uint8_t i = 0; i < 8; ++i)
            mblock_free_rng_aux(b, bg, ed, lvl-1,
                    n + i*mblock_page_size_lvl(lvl-1));
}
void mblock_free_rng(struct MemBlock* b, uint16_t begin, uint16_t end) {
    mblock_free_rng_aux(b, begin, end, 3, 0);
}


size_t mbtree_height_for(size_t nb_blocks) {
    size_t h = 0;
    for(size_t span=1; span < nb_blocks; span *= 64)
        ++h;
    return h;
}

size_t mbtree_intn_for(size_t h) {
    return ((1<<(6*h))-1) / 63;
}

size_t mbtree_space_for(size_t intn, size_t nb_blocks) {
    return ((63 + intn + nb_blocks  +63)/64) * 8;
}

void mbtree_init(struct MemBlockTree* t, size_t height, size_t intn,
        size_t space, uint64_t *cnt) {
    t->cnt     = cnt;
    t->lvs     = intn  + 63;
    kassert(t->lvs % 64 == 0, "mbt::lvs % 64");
    t->lvs_lim = space * 8;
    t->height  = height;

    for(size_t i = 0; 8*i < space; ++i) cnt[i] = 0;
}

static inline void mbt_set(uint64_t* c, size_t i) {
    c[i>>6] |= ((uint64_t)1)<<(i&0x3f);
}
static inline void mbt_clear(uint64_t* c, size_t i) {
    c[i>>6] &= ~(((uint64_t)1)<<(i&0x3f));
}
static inline uint64_t mbt_get(uint64_t* c, size_t i) {
    return c[i>>6] & (((uint64_t)1)<<(i&0x3f));
}
static inline size_t mbt_child(size_t i) {
    return (i-62)<<6;
}
static inline size_t mbt_parent(size_t i) {
    return (i>>6) + 62;
}
static inline uint64_t mbt_get_childs(uint64_t* c, size_t i) {
    return c[i - 62];
}
void mbtree_add(struct MemBlockTree* t, size_t i) {
    for(i += t->lvs; i>62; i = mbt_parent(i))
        mbt_set(t->cnt, i);
}
void mbtree_rem(struct MemBlockTree* t, size_t i) {
    i += t->lvs;
    mbt_clear(t->cnt, i);
    for (i = mbt_parent(i); i>62 && !mbt_get_childs(t->cnt, i);
            i = mbt_parent(i))
        mbt_clear(t->cnt, i);
}
uint8_t mbtree_get(struct MemBlockTree* t, size_t i) {
    return mbt_get(t->cnt, t->lvs + i) ? 1 : 0;
}
uint8_t mbtree_non_empty(struct MemBlockTree* t) {
    return (t->cnt[0]>>63) & 1;
}
size_t mbtree_find(struct MemBlockTree* t) {
    size_t i = 63;
    for (size_t h = 0; h < t->height; ++h)
        i = mbt_child(i) + find_bit_64(mbt_get_childs(t->cnt, i), 1, 6);
    return i - t->lvs;
}
void mbtree_updt_intn_add(struct MemBlockTree* t) {
    for (size_t i = t->lvs - 1; i > 62; --i)
        if (mbt_get_childs(t->cnt, i))
            mbt_set(t->cnt, i);
}
void mbtree_add_rng(struct MemBlockTree* t, size_t begin, size_t end) {
    for (size_t i = begin & ~((size_t)63); i < end; i += 64) {
        uint64_t v = ~(uint64_t)0;
        if (begin > i) v <<= begin - i;
        if (end < i + 64)
            v &= (~(uint64_t)0) >> (i + 64 - end);
        t->cnt[(t->lvs + i)>>6] = v;
    }
    mbtree_updt_intn_add(t);
}


void palloc_init(struct PageAllocator* a, struct MemBlock* mbs,
        size_t h, size_t intn, size_t spac,
        uint64_t* cnt0, uint64_t* cnt1) {

    a->mblocks = mbs;
    for (size_t i = 0; i < a->nb_blocks; ++i)
        mblock_init0(mbs + i);

    mbtree_init(&a->mbt_part,  h, intn, spac, cnt0);
    mbtree_init(&a->mbt_full,  h, intn, spac, cnt1);
}

void palloc_add_zones(struct PageAllocator* a,
        uint64_t lims[], size_t lims_sz) {
    size_t sg[2] = {0, 0};
    for (size_t i = 0; i < lims_sz - 1; ++i) {
        if (lims[i]&1)
            sg[(lims[i]>>1)&1]--;
        else
            sg[(lims[i]>>1)&1]++;

        if (sg[0] && !sg[1]) {
            phy_addr z_bg = lims[i]   & PAGE_MASK,
                     z_ed = lims[i+1] & PAGE_MASK;
            for (size_t mb = z_bg >> MBLOC_SHIFT;
                    (mb << MBLOC_SHIFT) < z_ed;
                    ++mb) {
                uint16_t zmb_bg = 0, zmb_ed = 512;
                if (z_bg > (mb << MBLOC_SHIFT))
                    zmb_bg = ((z_bg & MBLOC_OFS_MASK) >> PAGE_SHIFT);
                if (z_ed < ((mb+1) << MBLOC_SHIFT))
                    zmb_ed = ((z_ed & MBLOC_OFS_MASK) >> PAGE_SHIFT);
                mblock_free_rng(a->mblocks+mb, zmb_bg, zmb_ed);
            }
        }
    }

    for (size_t mb = 0; mb < a->nb_blocks; ++mb){
        if (mblock_full_free(a->mblocks + mb))
            mbtree_add(&a->mbt_full, mb);
        else if (mblock_non_empty(a->mblocks + mb))
            mbtree_add(&a->mbt_part, mb);
    }
}

static inline void a_swap(uint64_t* a, size_t i, size_t j) {
    uint64_t t = a[i];
    a[i] = a[j];
    a[j] = t;
}
//Quick sort
//|0  < p  |i   =p  |j  ?  |k   > p  |sz
void sort_limits(uint64_t* a, size_t sz) {
    if (sz <= 1) return;
    uint64_t p = a[sz / 2];
    size_t i = 0, j = 0, k = sz;
    while (j < k) {
        if (a[j] < p)
            a_swap(a, i++, j++);
        else if(a[j] > p)
            a_swap(a, j, --k);
        else
            ++j;
    }
    sort_limits(a,   i   );
    sort_limits(a+k, sz-k);
}

phy_addr palloc_alloc_page(struct PageAllocator* a) {
    size_t b;
    if (mbtree_non_empty(&a->mbt_part))
        b = mbtree_find(&a->mbt_part);
    else if(mbtree_non_empty(&a->mbt_full))
        b = mbtree_find(&a->mbt_full);
    else {
        kpanic("Out of memory");
        return 0; //Non atteint
    }

    uint16_t prev_3 = mblock_full_free (a->mblocks + b);
    uint16_t p_rel  = mblock_alloc_page(a->mblocks + b);
    if (prev_3) {
        mbtree_add(&a->mbt_part,  b);
        mbtree_rem(&a->mbt_full, b);
    } else if (!mblock_non_empty(a->mblocks+b))
        mbtree_rem(&a->mbt_part,  b);

    phy_addr rt = p_rel << PAGE_SHIFT;
    rt |= ((phy_addr)b) << MBLOC_SHIFT;
    rt += a->addr_begin;

    klogf(Log_verb, "mem", "alloc %h", rt);

    return rt;
}
void palloc_free_page(struct PageAllocator* a, phy_addr p) {
    phy_addr ofs = p - a->addr_begin;
    size_t   b   = ofs >> MBLOC_SHIFT;
    mblock_free_lvl_0(a->mblocks + b, ((p & MBLOC_OFS_MASK) >> PAGE_SHIFT));
    if (mblock_full_free(a->mblocks + b)) {
        mbtree_add(&a->mbt_full, b);
        mbtree_rem(&a->mbt_part,  b);
    } else
        mbtree_add(&a->mbt_part,  b);
}
size_t palloc_nb_free_page(struct PageAllocator* a) {
    size_t rt = 0;
    for (size_t b = 0; b < a->nb_blocks; ++b)
        rt += mblock_nb_page_free(a->mblocks + b);
    return rt;
}
