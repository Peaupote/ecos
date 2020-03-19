#include "shared_ptr.h"

#ifndef TEST_UNIT
#include "kmem.h"
#endif

sptra_node* spn_at(uint64_t a) {
	return & (&sptr_alct.entries)[a].node;
}

void sptra_init() {
	kassert(!kmem_paging_alloc((uint_ptr)&sptr_alct,
				PAGING_FLAG_R, PAGING_FLAG_R | PAGING_FLAG_G),
				"sptra page 0");
	kAssert(sizeof(union SPTRAllocator) == 16);
	kAssert(sizeof(union sptra_entry) == 16);
	sptr_alct.root = 0;
	sptr_alct.size = 0;
}


void sptra_add_btree(uint64_t tad,
		size_t lvl, uint64_t msk, uint64_t* ref) {
	uint64_t oth = *ref;
	sptra_node* stad = spn_at(tad);
	if (!oth) {
		stad->right = stad->left = 0;
		*ref = tad;
		return;
	}
	sptra_node* soth = spn_at(oth);
	kassert(lvl, "add lvl");
	kassert((tad & ~msk)==(oth & ~msk), "add ~mask");
	if ((tad>>lvl) & 1) {
		if (oth <= tad)//T@r
			sptra_add_btree(tad, lvl - 1, msk >> 1, &soth->right);
		else {//O@r
			stad->right = soth->right;
			kassert(!soth->left, "btree 01");
			stad->left  = 0;
			*ref = tad;
			sptra_add_btree(oth, lvl - 1, msk >> 1, &stad->right);
		}
	} else {
		if ((oth>>lvl) & 1) {//O@r
			stad->right = soth->right;
			kassert(!soth->left, "btree 10");
			stad->left  = 0;
			*ref = tad;
			sptra_add_btree(oth, lvl - 1, msk >> 1, &stad->right);
		} else if (oth <= tad) {//O@l
			stad->right = soth->right;
			stad->left  = soth->left;
			*ref = tad;
			sptra_add_btree(oth, lvl - 1, msk >> 1, &stad->left);
		} else //T@l
			sptra_add_btree(tad, lvl - 1, msk >> 1, &soth->left);
	}
}

void sptra_add_vtree(uint64_t tad,
		size_t lvl, uint64_t msk, uint64_t* ref) {
	uint64_t oth = *ref;
	sptra_node* stad = spn_at(tad);
	if (!oth) {
		stad->right = stad->left = 0;
		*ref = tad;
		return;
	}
	sptra_node* soth = spn_at(oth);
	uint64_t dsc = 1 << (lvl + 1);
	kassert(msk+1 == dsc, "msk");
	if (tad >= dsc) {
		if (oth >= dsc)//lvl++
			sptra_add_vtree(tad, lvl + 1, (msk << 1)|1, ref);
		else //T@r
			sptra_add_vtree(tad, lvl + 1, (msk << 1)|1, &soth->right);
	} else {
		if (oth >= dsc) {//O@r
			stad->right = oth;
			stad->left  = 0;
			*ref = tad;
		} else {
			kassert((oth >= (dsc>>1)) && (oth < dsc)
				&&  (tad >= (dsc>>1)) && (tad < dsc),
				"intv");
			if (oth <= tad) {//O@l
				stad->right = soth->right;
				stad->left  = soth->left;
				*ref = tad;
				sptra_add_btree(oth, lvl - 1, msk >> 1, &stad->left);
			} else //T@l
				sptra_add_btree(tad, lvl - 1, msk >> 1, &soth->left);
		}
	}
}

void sptra_add_tree(uint64_t tad) {
	sptra_add_vtree(tad, 0, 0x1, &sptr_alct.root);
}

uint8_t sptra_find_ref_rec(uint64_t tfd, uint64_t** rt, uint8_t st,
		uint64_t* ref) {
	uint64_t oth = *ref;
	if (oth == tfd) {
		*rt = ref;
		return st;
	}
	if (!oth) return 0;
	if (oth < tfd)
		return sptra_find_ref_rec(tfd, rt, st, &spn_at(oth)->right);
	else
		return sptra_find_ref_rec(tfd, rt,  1, &spn_at(oth)->left);
}

uint8_t sptra_find_ref(uint64_t tfd, uint64_t** rt) {
	return sptra_find_ref_rec(tfd, rt, 2, &sptr_alct.root);
}

uint64_t* sptra_rightmost(uint64_t* rt) {
	sptra_node* srt = spn_at(*rt);
	return srt->right ? sptra_rightmost(&srt->right) : rt;
}
uint64_t* sptra_leftmost(uint64_t* rt) {
	sptra_node* srt = spn_at(*rt);
	return srt->left ? sptra_leftmost(&srt->left) : rt;
}

#define REM_SWAP(U, V) \
	uint64_t  b  = *rb;\
	sptra_node* sb = spn_at(b);\
	if (rb == &sa->U) {\
		sa->U = sb->U;\
		sb->U = a;\
		sb->V = sa->V;\
		sa->V = 0;\
		*ra = b;\
		sptra_rem_btree(&sb->U);\
	} else {\
		uint64_t tmp = sb->U;\
		sb->U = sa->U;\
		sa->U = tmp;\
		sb->V = sa->V;\
		sa->V = 0;\
		*rb   = a;\
		*ra   = b;\
		sptra_rem_btree(rb);\
	}

void sptra_rem_btree(uint64_t* ra) {
	uint64_t a = *ra;
	sptra_node* sa = spn_at(a);
	if (sa->left) {
		uint64_t* rb = sptra_rightmost(&sa->left);
		REM_SWAP(left, right);
	} else if (sa->right){
		uint64_t* rb = sptra_leftmost(&sa->right);
		REM_SWAP(right, left);
	} else *ra = 0;
}
void sptra_rem_vtree(uint64_t* ra) {
	uint64_t a = *ra;
	sptra_node* sa = spn_at(a);
	if (sa->left) {
		uint64_t* rb = sptra_rightmost(&sa->left);
		REM_SWAP(left, right);
	} else *ra = sa->right;
}
void sptra_rem_tree(uint64_t trm) {
	uint64_t* ref;
	uint8_t   st = sptra_find_ref(trm, &ref);
	kassert(st, "find rem");
	if (st==2)
		sptra_rem_vtree(ref);
	else
		sptra_rem_btree(ref);
}

uint8_t sptra_check_btree(uint64_t i, size_t lvl,
		uint64_t topb, uint64_t* l, uint64_t* r) {
	if ((i>>(lvl+1)) != (topb>>(lvl+1))) return 0;
	sptra_node* c = spn_at(i);
	uint64_t lr, rl;
	if (*l > i) *l = i;
	if (*r < i) *r = i;
	if (c->left) {
		lr = c->left;
		if (!sptra_check_btree(c->left, lvl-1, topb, l, &lr))
			return 0;
		if (lr >= i) return 0;
	}
	if (c->right) {
		rl = c->right;
		if (!sptra_check_btree(c->right, lvl-1, topb|(1<<lvl), &rl, r))
			return 0;
		if (rl <= i) return 0;
	}
	return 1;
}
bool sptra_check_vtree(uint64_t i, size_t lvl) {
	if (i >> (lvl+1)) return sptra_check_vtree(i, lvl+1);
	sptra_node* c = spn_at(i);
	if (c->left) {
		uint64_t l = c->left, r = c->left;
		if (!sptra_check_btree(c->left, lvl-1, 1<<lvl, &l, &r))
			return 0;
		if (!(l>>lvl) || r >= i) return 0;
	}
	return !c->right || sptra_check_vtree(c->right, lvl+1);
}
bool sptra_check_tree() {
	return !sptr_alct.root
		||	sptra_check_vtree(sptr_alct.root, 0);
}
size_t sptra_size_tree(uint64_t root) {
	return root ?
			(1 + sptra_size_tree(spn_at(root)->left)
			  + sptra_size_tree(spn_at(root)->right))
			: 0;
}

struct sptr_hd* sptr_at(uint64_t a) {
	return & (&sptr_alct.entries)[a].info;
}

uint64_t sptr_alloc() {
	if (sptr_alct.root) {
		sptra_node* srt = spn_at(sptr_alct.root);
		if (srt->left) {
			uint64_t* rtr = sptra_leftmost(&srt->left);
			uint64_t  rt  = *rtr;
			sptra_rem_btree(rtr);
			return rt;
		} else {
			uint64_t rt = sptr_alct.root;
			sptr_alct.root = srt->right;
			return rt;
		}
	} else {
		uint64_t rt = sptr_alct.size + 1;
		uint_ptr rtpt = rt*sizeof(union sptra_entry);
		if ((rtpt^(sptr_alct.size*sizeof(union sptra_entry))) & PAGE_MASK)
			kassert(!kmem_paging_alloc(
					((uint_ptr)&sptr_alct) + (rtpt&PAGE_MASK),
					PAGING_FLAG_R, PAGING_FLAG_R | PAGING_FLAG_G),
					"sptra page");
		return sptr_alct.size = rt;
	}
}
void sptr_cut_tree_right() {
	while (true) {
		uint64_t* rtm = sptra_rightmost(&sptr_alct.root);
		if (! *rtm) return;
		do {
			kAssert(*rtm <= sptr_alct.size);
			if (*rtm < sptr_alct.size) return;
			--sptr_alct.size;
			sptra_rem_vtree(rtm);
		} while(*rtm);
	}
}
void sptr_free(uint64_t idx) {
	if (idx == sptr_alct.size) {
		uint_ptr prev_pg = (sizeof(union sptra_entry) * sptr_alct.size--)
							& PAGE_MASK;
		sptr_cut_tree_right();
		for(uint_ptr new_pg = sizeof(union sptra_entry) * sptr_alct.size;
				prev_pg > new_pg; prev_pg -= PAGE_SIZE)
			kmem_paging_free(((uint_ptr)&sptr_alct) + prev_pg);
	} else
		sptra_add_tree(idx);
}
size_t sptr_nb_alloc() {
	return sptr_alct.size - sptra_size_tree(sptr_alct.root);
}

