#include "page_alloc.h"

#define QD_0111_MASK 0x77777777
#define QD_1000_MASK 0x88888888

void mblock_init(struct MemBlock* b, phy_addr a) {
	for(uint64_t* it = (uint64_t*)b; 
			it != ((uint64_t*)b) + sizeof(struct MemBlock) / 8; ++it)
		*it = 0;
	b->nb_at_lvl[3] = 1;
	b->addr = a;
}

static inline uint8_t find_bit(uint64_t p, uint8_t sz, uint8_t step) {
	uint64_t p2;
	uint8_t  i = 0;

	for (uint8_t j = step; j--;) {
		p2 = p >> (sz << j);
		i |= p2 ? (1<<j) : 0;
		p  = p2 ? p2     : p;
	}

	return i;
}
static inline uint8_t find_in_8(uint64_t p, uint8_t sz) {
	return find_bit(p, sz, 3);
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

	n = l2 = find_in_8(b->lvl_2_0, 8);
	--(((uint8_t*)&b->lvl_2_0)[l2]);

	l1 = find_in_8(b->lvl_1_10[n] & QD_0111_MASK, 4);
	decr_qd(b->lvl_1_10 + n, l1);
	n  = (n<<3) | l1;

	l0 = find_in_8(b->lvl_0_0 [n], 1);
	b->lvl_0_0 [n] &= ~bit_1(l0);
	n  = (n<<3) | l0;

	return n;
}

uint16_t mblock_alloc_lvl_1(struct MemBlock* b) {
	uint16_t n;
	uint8_t  l1, l2;

	--b->nb_at_lvl[1];

	n = l2 = find_in_8(b->lvl_2_21 & QD_0111_MASK, 4);
	decr_qd(&b->lvl_2_21, l2);

	l1 = find_in_8(b->lvl_1_10[n] & QD_1000_MASK, 4);
	b->lvl_1_10 [n] &= ~bit_4(l1);
	n  = (n<<3) | l1;

	return n<<3;
}

uint16_t mblock_alloc_lvl_2(struct MemBlock* b) {
	uint16_t n;
	uint8_t  l2;

	--b->nb_at_lvl[2];

	n = l2 = find_in_8(b->lvl_2_21 & QD_1000_MASK, 4);
	b->lvl_2_21 &= ~bit_4(l2);

	return n<<6;
}

void mblock_alloc_lvl_3(struct MemBlock* b) {
	--b->nb_at_lvl[3];
}


void mblock_free_lvl_3(struct MemBlock* b) {
	++b->nb_at_lvl[3];
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
	return (63 + intn + nb_blocks  +63)/64;
}

void mbtree_init(struct MemBlockTree* t, size_t height, size_t intn,
		size_t space, uint64_t *cnt) {
	t->cnt     = cnt;
	t->lvs     = intn  + 63;
	t->lvs_lim = space * 8;
	t->height  = height;

	for(size_t i=0; i<space; ++i) cnt[i] = 0;
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
	for(i = mbt_parent(i); i>62 && !mbt_get_childs(t->cnt, i);
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
	for(size_t h = 0; h < t->height; ++h)
		i = mbt_child(i) + find_bit(mbt_get_childs(t->cnt, i), 1, 6);
	return i - t->lvs;
}