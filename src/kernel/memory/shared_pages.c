#include <kernel/memory/shared_pages.h>

#include <kernel/memory/shared_ptr.h>
#include <kernel/memory/kmem.h>

static inline bool is_P1_ShRo(uint64_t p) {
	return (p & (SPAGING_FLAG_P|SPAGING_FLAG_V))
			== SPAGING_FLAG_V;
}

static inline bool is_P0_Value(uint64_t p) {
	return (p & (SPAGING_FLAG_P | SPAGING_FLAG_V)) 
			== (SPAGING_FLAG_P | SPAGING_FLAG_V);
}

static inline void 
bind_and_follow(uint64_t* e, uint64_t* a, phy_addr p) {
	*e = p | PAGING_FLAG_P | (SPAGING_FLAGS_1 & *e);
	*a = paging_rm_loop(*a);
	invalide_page(*a);
}

void copy_page(uint64_t* src, uint64_t* dst, uint16_t lim) {
	for (uint16_t i = 0; i < lim; ++i) {

		if (src[i] & PAGING_FLAG_P) {

			if (is_P1_ShRo(src[i]))
				dst[i] = src[i];
			else {
				uint64_t        sp_idx;
				struct sptr_hd* sp_hd;
				dst[i] = kmem_mk_shared(src + i, &sp_idx, &sp_hd);
				sp_hd->count = 2;
			}

		} else if (is_P0_Value(src[i])) { 

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

	for(enum pgg_level lvl = pgg_pml4; lvl != 0;) { --lvl;
		uint64_t* query = (uint64_t*) query_addr;
		uint64_t  qe = *query;
		if(! (qe & PAGING_FLAG_P) ){
			if (! (qe & (SPAGING_FLAG_P | SPAGING_FLAG_V)) ) {
				klogf(Log_error, "#PF", "handling fail @ lvl %d : %p",
						(int)lvl, qe);
				return ~(uint8_t)0;
			}

			if (is_P0_Value(qe)) {//Page partagée
				uint64_t idx = qe >> SPAGING_INFO_SHIFT;
				struct sptr_hd* hd = sptr_at(idx);
				if (hd->count <= 1) {

					bind_and_follow(query, &query_addr,
							hd->addr & PAGE_MASK);
					sptr_free(idx);

				} else {
					klogf(Log_verb, "mem", "alloc copy for lvl %d", lvl);

					phy_addr new_struct = kmem_alloc_page();
					if (!lvl)
						*query |= PAGING_FLAG_W;
					bind_and_follow(query, &query_addr, new_struct);
					
					--hd->count;
					kmem_bind_dynamic_slot(0, hd->addr & PAGE_MASK);
					uint64_t* src = (uint64_t*)kmem_dynamic_slot(0);
					uint64_t* dst = (uint64_t*)query_addr;
					if (lvl)
						copy_page(src, dst, PAGE_ENT);
					else {
						copy_page0(src,dst);
						if (! (qe & PAGING_FLAG_W) )
							*query &= ~(uint64_t)PAGING_FLAG_W;
					}

				}

			} else {//Nouvelle page
				klogf(Log_verb, "mem", "alloc new for lvl %d", lvl);

				phy_addr new_struct = kmem_alloc_page();
				bind_and_follow(query, &query_addr, new_struct);
				if (lvl) {
					// S'il s'agit d'une nouvelle structure de paging,
					// on recopie les flags
					for(uint16_t i = 0; i < PAGE_ENT; ++i)
						((uint64_t*) query_addr)[i] = qe;
				} else if (qe & SPAGING_FLAG_V) {
					// Alloc0
					for(uint16_t i = 0; i < PAGE_ENT; ++i)
						((uint64_t*) query_addr)[i] = 0;
				}
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

void kmem_free_rec(uint64_t* entry, enum pgg_level lvl);
void kmem_free_paging_range(uint64_t* page_bg,
			enum pgg_level lvl, uint16_t lim) {

	for (uint16_t i = 0; i < lim; ++i) {

		if (page_bg[i] & PAGING_FLAG_P) {
			if (!is_P1_ShRo(page_bg[i]))
				kmem_free_rec(page_bg + i, lvl - 1);

		} else if (is_P0_Value(page_bg[i])) {

			uint64_t       idx = page_bg[i] >> SPAGING_INFO_SHIFT;
			struct sptr_hd* hd = sptr_at(idx);

			if (hd->count <= 1) {
				page_bg[i] = (hd->addr & PAGE_MASK)
				           | PAGING_FLAG_W | PAGING_FLAG_P;
				invalide_page(paging_rm_loop((uint64_t)(page_bg+i)));
				sptr_free(idx);
				kmem_free_rec(page_bg + i, lvl - 1);
			} else --hd->count;

		}

		page_bg[i] = 0;
	}
}
void kmem_free_rec(uint64_t* entry, enum pgg_level lvl) {
	if (lvl) {
		uint64_t* page_bg = (uint64_t*) paging_rm_loop((uint_ptr)entry);
		kmem_free_paging_range(page_bg, lvl, PAGE_ENT);
	}
	kmem_free_page((*entry) & PAGE_MASK);
}
void kmem_free_paging(phy_addr old_pml4, phy_addr new_pml4) {
	kmem_free_paging_range(paging_acc_pml4(0), pgg_pml4, PML4_END_USPACE);

	pml4_to_cr3(new_pml4);

	kmem_free_page(old_pml4);

	klogf(Log_verb, "mem", "%lld pages disponibles",
			(long long int)kmem_nb_page_free());
}
