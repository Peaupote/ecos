#include <kernel/memory/shared_pages.h>

#include <kernel/memory/shared_ptr.h>
#include <kernel/memory/kmem.h>

#define PAGING_FLAG0 (PAGING_FLAG_U | PAGING_FLAG_W | PAGING_FLAG_P)

static inline void 
bind_and_follow(uint64_t* e, uint64_t* a, phy_addr p) {
	*e = p | PAGING_FLAG0;
	*a = paging_rm_loop(*a);
	invalide_page(*a);
}

void copy_page(uint64_t* src, uint64_t* dst, uint16_t lim) {
	for (uint16_t i = 0; i < lim; ++i) {
		if (src[i] & PAGING_FLAG_P) {
			uint64_t idx = sptr_alloc();
			struct sptr_hd* hd = sptr_at(idx);
			hd->count = 2;
			hd->addr  = src[i] & PAGE_MASK;
			dst[i] = src[i] = (idx << SPAGING_INFO_SHIFT)
				   | SPAGING_FLAG_P | SPAGING_FLAG_V;
		} else if ((src[i] & SPAGING_FLAGS) 
				== (SPAGING_FLAG_P | SPAGING_FLAG_V)) {
			uint64_t idx = src[i] >> SPAGING_INFO_SHIFT;
			++sptr_at(idx)->count;
			dst[i] = src[i];
		} else
			dst[i] = src[i];
	}
}
void copy_page0(uint64_t* src, uint64_t* dst) {
	for (uint16_t i = 0; i < PAGE_ENT; ++i)
		dst[i] = src[i];
}

uint8_t handle_PF(uint_ptr fault_addr) {
	uint64_t query_addr =
		(uint64_t) paging_acc_pml4(paging_get_lvl(pgg_pml4, fault_addr));

	for(uint8_t lvl = 4; lvl != 0;) { --lvl;
		uint64_t* query = (uint64_t*) query_addr;
		uint64_t  qe = *query;
		if(! (qe & PAGING_FLAG_P) ){
			if (! (qe & SPAGING_FLAG_P) ) return ~(uint8_t)0;
			if (qe & SPAGING_FLAG_V) {//Page partagée
				uint64_t idx = qe >> SPAGING_INFO_SHIFT;
				struct sptr_hd* hd = sptr_at(idx);
				if (hd->count <= 1) {
					bind_and_follow(query, &query_addr,
							hd->addr & PAGE_MASK);
					sptr_free(idx);
				} else {
					klogf(Log_verb, "mem", "alloc copy for lvl %d", lvl);
					phy_addr new_struct = kmem_alloc_page();
					bind_and_follow(query, &query_addr, new_struct);
					--hd->count;
					kmem_bind_dynamic_slot(0, hd->addr & PAGE_MASK);
					uint64_t* src = (uint64_t*)kmem_dynamic_slot(0);
					uint64_t* dst = (uint64_t*)query_addr;
					if (lvl)
						copy_page(src, dst, PAGE_ENT);
					else
						copy_page0(src,dst);
					klogf(Log_verb, "mem", "end copy lvl %d", lvl);
				}
			} else {//Nouvelle page
				klogf(Log_verb, "mem", "alloc new for lvl %d", lvl);
				phy_addr new_struct = kmem_alloc_page();
				bind_and_follow(query, &query_addr, new_struct);
				if (lvl)
					for(uint16_t i = 0; i < PAGE_ENT; ++i)
						((uint64_t*) query_addr)[i] = qe;
			}
		} else
			query_addr = paging_rm_loop(query_addr);
		query_addr |= (fault_addr >> (lvl * 9)) & PAGE_ENT_MASK;
	}

	return 0;
}

void kmem_fork_paging(phy_addr new_pml4) {
	kmem_bind_dynamic_slot(0, new_pml4);
	uint64_t* src = paging_acc_pml4(0);
	uint64_t* dst = (uint64_t*)kmem_dynamic_slot(0);
	
	copy_page(src, dst,	PML4_END_USPACE);
	
	for (uint16_t i = PML4_END_USPACE; i < PAGE_ENT; ++i)
		dst[i] = src[i];

	dst[PML4_LOOP] = new_pml4 | PAGING_FLAG_W | PAGING_FLAG_P;
	
	paging_refresh();
}

void kmem_free_rec(uint64_t* entry, uint8_t lvl);
void kmem_free_loop(uint64_t* page_bg,
			uint8_t lvl, uint16_t lim) {
	for (uint16_t i = 0; i < lim; ++i) {
		if (page_bg[i] & PAGING_FLAG_P)
			kmem_free_rec(page_bg + i, lvl - 1);
		else if ((page_bg[i] & SPAGING_FLAGS)
				== (SPAGING_FLAG_P | SPAGING_FLAG_V)) {
			uint64_t idx = page_bg[i] >> SPAGING_INFO_SHIFT;
			struct sptr_hd* hd = sptr_at(idx);
			if (hd->count <= 1) {
				page_bg[i] = (hd->addr & PAGE_MASK) | PAGING_FLAG0;
				invalide_page(paging_rm_loop((uint64_t)(page_bg+i)));
				sptr_free(idx);
				kmem_free_rec(page_bg + i, lvl - 1);
			} else --hd->count;
		}
	}
}
void kmem_free_rec(uint64_t* entry, uint8_t lvl) {
	if (lvl) {
		uint64_t* page_bg = (uint64_t*) paging_rm_loop((uint_ptr)entry);
		kmem_free_loop(page_bg, lvl, PAGE_ENT);
	}
	kmem_free_page((*entry) & PAGE_MASK);
}
void kmem_free_paging(phy_addr old_pml4, phy_addr new_pml4) {
	kmem_free_loop(paging_acc_pml4(0), 4, PML4_END_USPACE);

	pml4_to_cr3(new_pml4);

	kmem_free_page(old_pml4);

	klogf(Log_verb, "mem", "%lld pages disponibles",
			(long long int)kmem_nb_page_free());
}
