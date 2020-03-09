#include "kmem.h"

#include<stddef.h>

#include "../../def.h"
#include "page_alloc.h"
#include "../kutil.h"

// Accès aux 2MB d'adresses basses
uint64_t laddr_pd[512] __attribute__ ((aligned (PAGE_SIZE)));

phy_addr first_bloc;
struct MemBlock     mblocks[10];
struct MemBlockTree mbt_part;
struct MemBlockTree mbt_full;
uint64_t mbt_cnt[5 * 2];


//TODO: detection
void kmem_init_alloc() {
	// On cherche le prochain bloc disponible
	first_bloc = (phy_addr) end_kernel;
	if (first_bloc & MBLOC_OFS_MASK)
		first_bloc = (first_bloc & MBLOC_MASK) + MBLOC_SIZE;

	{
		size_t h    = mbtree_height_for(10);
		size_t intn = mbtree_intn_for(h);
		size_t spac = mbtree_space_for(intn, 10);// < 5
		mbtree_init(&mbt_part,  h, intn, spac, mbt_cnt);
		mbtree_init(&mbt_full, h, intn, spac, mbt_cnt + 5);
	}

	for(size_t i=0; i < 10; ++i) {
		mblock_init(mblocks + i, first_bloc + i * MBLOC_SIZE);
		mbtree_add(&mbt_full, i);
	}
}

void kmem_init() {
	laddr_pd[0] = 0 | PAGING_FLAG_S | PAGING_FLAG_R | PAGING_FLAG_P;
	for(uint16_t i=1; i<512; ++i)
		laddr_pd[i] = PAGING_FLAG_R;
	*paging_acc_pdpt(PML4_KERNEL_VIRT_ADDR, KERNEL_PDPT_LADDR)
		= paging_phy_addr_page((uint_ptr)laddr_pd)
		| PAGING_FLAG_R | PAGING_FLAG_P;

	kmem_init_alloc();
}

phy_addr kmem_alloc_page() {
	size_t b;
	if (mbtree_non_empty(&mbt_part))
		b = mbtree_find(&mbt_part);
	else if(mbtree_non_empty(&mbt_full))
		b = mbtree_find(&mbt_full);
	else {
		kpanic("Out of memory");
		return 0; //Non atteint
	}
	uint16_t prev_3  = mblocks[b].nb_at_lvl[3];
	uint16_t p_rel   = mblock_alloc_page(mblocks+b);
	if(prev_3) {
		mbtree_add(&mbt_part,  b);
		mbtree_rem(&mbt_full, b);
	} else if(! *((uint64_t*)mblocks[b].nb_at_lvl))
		mbtree_rem(&mbt_part,  b);
	return mblocks[b].addr | (p_rel<<PAGE_SHIFT);
}

void kmem_free_page(phy_addr p) {
	phy_addr ofs = p - first_bloc;
	size_t   b   = ofs >> MBLOC_SHIFT;
	mblock_free_lvl_0(mblocks + b, ((p & MBLOC_OFS_MASK) >> PAGE_SHIFT));
	if(mblocks[b].nb_at_lvl[3]) {
		mbtree_add(&mbt_full, b);
		mbtree_rem(&mbt_part,  b);
	} else
		mbtree_add(&mbt_part,  b);
}

uint64_t* acc_pt_entry(uint_ptr v_addr, uint16_t flags) {
	uint64_t query_addr = 
		(uint64_t) paging_acc_pml4(paging_get_pml4(v_addr));
	
	for(uint8_t i=0; i<3; i++) {
		uint64_t* query = (uint64_t*) query_addr;
		if(! ((*query) & PAGING_FLAG_P) ){
			phy_addr new_struct = kmem_alloc_page();
			*query = new_struct | flags | PAGING_FLAG_P;

			query_addr = (query_addr << 9) & VADDR_MASK;
			for(uint16_t i=0; i<512; ++i)
				((uint64_t*) query_addr)[i] = PAGING_FLAG_R;
		} else if((*query) & PAGING_FLAG_S)
			return NULL;
		else {
			*query |=  flags;
			query_addr = paging_rm_loop(query_addr);
		}
		query_addr |= (v_addr >> ((3-i) * 9)) & 0xff8;
	}

	return (uint64_t*) query_addr;
}

uint8_t paging_map_to(uint_ptr v_addr, phy_addr p_addr){
	uint64_t* query = acc_pt_entry(v_addr, PAGING_FLAG_R);
	if (!query) return ~0;
	if ( (*query) & PAGING_FLAG_P) return 1; //Déjà assignée
	*query = p_addr | PAGING_FLAG_R | PAGING_FLAG_P;
	return 0;
}

uint8_t paging_force_map_to(uint_ptr v_addr, phy_addr p_addr){
	uint64_t* query = acc_pt_entry(v_addr, PAGING_FLAG_R);
	if (!query) return ~0;
	*query = p_addr | PAGING_FLAG_R | PAGING_FLAG_P;
	paging_refresh();
	return 0;
}

uint8_t kmem_paging_alloc(uint_ptr v_addr, uint16_t flags){
	uint64_t* query = acc_pt_entry(v_addr, flags);
	if (!query) return ~0;
	if ( (*query) & PAGING_FLAG_P) {
		*query |= flags;
		return 1;
	}
	*query = kmem_alloc_page() | flags | PAGING_FLAG_P;
	return 0;
}

void kmem_init_pml4(uint64_t* pml4, phy_addr p_loc) {
	for(uint16_t i=0; i<512; ++i)
		pml4[i] = PAGING_FLAG_R;
	pml4[PML4_KERNEL_VIRT_ADDR] = *paging_acc_pml4(PML4_KERNEL_VIRT_ADDR) 
		& (PAGE_MASK | PAGING_FLAG_P | PAGING_FLAG_R);
	pml4[PML4_LOOP] = p_loc | PAGING_FLAG_R | PAGING_FLAG_P;
}

void kmem_copy_pml4(uint64_t* pml4, phy_addr p_loc) {
	for(uint16_t i=0; i<512; ++i)
		pml4[i] = *paging_acc_pml4(i) 
			& (PAGE_MASK | PAGING_FLAGS);
	pml4[PML4_LOOP] = p_loc | PAGING_FLAG_R | PAGING_FLAG_P;
}

void kmem_copy_rec(uint64_t* dst_entry, uint64_t* src_page, uint8_t lvl) {
	phy_addr pg_phy = kmem_alloc_page();
	*dst_entry = ((*dst_entry) & PAGE_OFS_MASK) | pg_phy;
	uint64_t* dst_page = (uint64_t*) paging_rm_loop((uint_ptr)dst_entry);
	if (lvl)
		for (size_t i = 0; i < 512; ++i) {
			dst_page[i] = src_page[i];
			if (dst_page[i] & PAGING_FLAG_P)
				kmem_copy_rec(
						dst_page+i,
						(uint64_t*) paging_rm_loop((uint_ptr)(src_page+i)),
						lvl - 1);
		}
	else
		for (size_t i = 0; i < 512; ++i)
			dst_page[i] = src_page[i];
}

void kmem_copy_paging(volatile phy_addr new_pml4) {
	*paging_acc_pml4(PML4_COPY_RES) = new_pml4 
									| PAGING_FLAG_R | PAGING_FLAG_P;
	paging_refresh();

	kmem_copy_pml4(paging_pts_acc(
				PML4_LOOP, PML4_LOOP, PML4_LOOP,PML4_COPY_RES, 0),
				new_pml4);
	
	pml4_to_cr3(new_pml4);
	
	for (int i = 0; i < PML4_KERNEL_VIRT_ADDR; ++i)
		if(*paging_acc_pml4(i) & PAGING_FLAG_P) {
			*paging_acc_pml4(PML4_COPY_RES) 
				= *paging_acc_pml4(i) & ~PAGING_FLAG_U;
			paging_refresh();
			kmem_copy_rec(paging_acc_pml4(i),
					paging_acc_pdpt(PML4_COPY_RES,0), 3);
		}
}
