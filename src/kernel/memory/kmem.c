#include <kernel/memory/kmem.h>

#include <stddef.h>

#include <def.h>
#include <util/multiboot.h>
#include <kernel/kutil.h>
#include <kernel/tty.h>
#include <util/misc.h>

#include <kernel/memory/page_alloc.h>
#include <kernel/memory/shared_ptr.h>

//Taille maximale du tas de kernel (nombre de pages)
#define KHEAP_SZ 0x40000


phy_addr kernel_pml4;

// Emplacements dynamiques
uint64_t dslot_pt[512] __attribute__ ((aligned (PAGE_SIZE)));

// Heap: stocke les structures du kernel
// de tailles déterminiées à l'éxécution
uint64_t heap_pd [512] __attribute__ ((aligned (PAGE_SIZE)));
uint64_t heap_pt0[512] __attribute__ ((aligned (PAGE_SIZE)));
// Allocation des adresses virtuelles du tas
struct MemBlockTree  khep_alloc;
// Allocation des pages physiques
struct PageAllocator page_alloc;

static size_t total_nb_pages;


void kmem_init_paging() {
	kernel_pml4 = (*paging_acc_pml4(PML4_LOOP)) & PAGE_MASK;
	uint64_t* kpml4 = (uint64_t*) kernel_pml4;
	kpml4[0] = 0; // On enlève le mapping identité

	heap_pd [0] = paging_phy_addr_page((uint_ptr)heap_pt0)
                | PAGING_FLAG_W | PAGING_FLAG_P;

	*paging_acc_pdpt(PML4_KERNEL_VIRT_ADDR, KERNEL_PDPT_DSLOT)
		= paging_phy_addr_page((uint_ptr) dslot_pt)
		| PAGING_FLAG_W | PAGING_FLAG_P;

	*paging_acc_pdpt(PML4_KERNEL_VIRT_ADDR, KERNEL_PDPT_HEAP)
		= paging_phy_addr_page((uint_ptr) heap_pd)
		| PAGING_FLAG_W | PAGING_FLAG_P;
}


void kmem_init_alloc(multiboot_info_t* mbi) {
	//On récupère la carte de la mémoire fournie par GRUB
	if(! (mbi->flags & MULTIBOOT_INFO_MEM_MAP)) kpanic("mem map");

	phy_addr mmap_addr = mbi->mmap_addr;
	size_t mmap_length = mbi->mmap_length;
	if (mmap_length > 511 * PAGE_SIZE) kpanic("mem map size");

	kmem_bind_dynamic_range(0, mmap_addr, mmap_addr + mmap_length);
	uint8_t* mmap = (uint8_t*) kmem_dynamic_slot_at(0, mmap_addr);

	// Calcul de l'interval couvert
	page_alloc.addr_begin = ~(phy_addr)0;
	page_alloc.addr_end   = 0;
	size_t nb_mmape = 0;
	for(size_t it = 0; it < mmap_length;) {
		multiboot_memory_map_t* e = (multiboot_memory_map_t*) (mmap + it);
		klogf(Log_verb, "mem", "it=%d type=%d addr=%p len=%p",
				it, (int)e->type, (uint64_t)e->addr, (uint64_t)e->len);
		if (e->type == MULTIBOOT_MEMORY_AVAILABLE) {
			if (e->addr < page_alloc.addr_begin)
				page_alloc.addr_begin = e->addr;
			if (e->addr + e->len > page_alloc.addr_end)
				page_alloc.addr_end = e->addr + e->len;
		}
		++nb_mmape;
		it += e->size + sizeof(e->size);
	}
	page_alloc.addr_begin &= MBLOC_MASK;
	page_alloc.addr_end    = align_to(page_alloc.addr_end, MBLOC_SIZE);
	page_alloc.nb_blocks   = (page_alloc.addr_end - page_alloc.addr_begin)
							      / MBLOC_SIZE;

	// Calcul l'espace nécessaire pour les structures d'allocation
	size_t s_space     = page_alloc.nb_blocks * sizeof(struct MemBlock);
	size_t h           = mbtree_height_for(page_alloc.nb_blocks);
	size_t intn        = mbtree_intn_for(h);
	size_t t_spac1     = mbtree_space_for(intn, page_alloc.nb_blocks);
	size_t t_space_bg  = align_to(s_space, 8);
	size_t t_space     = 2 * t_spac1;
	s_space            = t_space_bg + t_space;
	size_t hp_space_bg = align_to(s_space, 8);
	size_t hp_h        = mbtree_height_for(KHEAP_SZ);
	size_t hp_intn     = mbtree_intn_for(hp_h);
	size_t hp_space    = mbtree_space_for(hp_intn, KHEAP_SZ);
	s_space = hp_space_bg + hp_space;

	if (s_space > 512 * PAGE_SIZE) kpanic("s_space");

	// On cherche un emplacement pour stocker les structures
	phy_addr ek_pg = align_to((phy_addr) end_kernel, PAGE_SIZE);
	phy_addr s_pya = (ek_pg + s_space > mmap_addr)
		? align_to(mmap_addr + mmap_length, PAGE_SIZE)
		: ek_pg;

	// On map cet emplacement sur le tas
	size_t heap_next_page = 0;
	for (; heap_next_page * PAGE_SIZE < s_space;
			++heap_next_page)
		heap_pt0[heap_next_page] = (s_pya + heap_next_page * PAGE_SIZE)
			    | PAGING_FLAG_W | PAGING_FLAG_P | PAGING_FLAG_G;
	
	uint8_t* s_space_bg  = (uint8_t*) paging_pts_acc
				(PML4_KERNEL_VIRT_ADDR, KERNEL_PDPT_HEAP, 0, 0, 0);

	palloc_init(&page_alloc,
		(struct MemBlock*) s_space_bg, h, intn, t_spac1,
		(uint64_t*)(s_space_bg + t_space_bg),
		(uint64_t*)(s_space_bg + t_space_bg + t_spac1));
	mbtree_init(&khep_alloc, hp_h, hp_intn, hp_space,
		(uint64_t*)(s_space_bg + hp_space_bg));
	mbtree_add_rng(&khep_alloc, heap_next_page, KHEAP_SZ);

	// Initialisation des MemBlocks
	//sizeof(multiboot_memory_map_t) + sizeof(e.size) >= 20 + 4 = 24
	//on transforme chaque entrée en 2 limites de 8 octets
	//on rajoute 2 entrées (kernel et structures d'alocation)
	// -> on réutilise l'emplacement du multiboot si au moins 8 entrées
	//    24 * n >= 16*n + 2*2*8 <=> n >= 4

	size_t    lims_sz = 0;
	uint64_t  space_for_lims[(3 + 2)*2];
	uint64_t* lims = nb_mmape >= 4 ? ((uint64_t*)mmap) : space_for_lims;

	for(size_t it = 0; it < mmap_length;) {
		multiboot_memory_map_t* e = (multiboot_memory_map_t*) (mmap + it);
		it += e->size + sizeof(e->size);
		uint64_t lim_bg = e->addr;
		uint64_t lim_ed = e->addr + e->len;
		if (e->type == MULTIBOOT_MEMORY_AVAILABLE) {
			lim_bg  = align_to(lim_bg, PAGE_SIZE);
			lim_ed &= PAGE_MASK;
			if (lim_bg >= lim_ed) continue;
			lim_ed |= 1;
		} else {
			lim_bg &= PAGE_MASK;
			lim_ed  = align_to(lim_ed, PAGE_SIZE);
			if (lim_bg < page_alloc.addr_begin)
				lim_bg = page_alloc.addr_begin;
			if (lim_ed > page_alloc.addr_end)
				lim_ed = page_alloc.addr_end;
			if (lim_bg >= lim_ed) continue;
			lim_bg |= 2;
			lim_ed |= 3;
		}
		lims[lims_sz++] = lim_bg - page_alloc.addr_begin;
		lims[lims_sz++] = lim_ed - page_alloc.addr_begin;
	}

	lims[lims_sz++] = (bgn_kernel & PAGE_MASK) | 2;
	lims[lims_sz++] = ek_pg | 3;
	lims[lims_sz++] = s_pya | 2;
	lims[lims_sz++] = align_to(s_pya + s_space, PAGE_SIZE) | 3;

	sort_limits(lims, lims_sz);
	palloc_add_zones(&page_alloc, lims, lims_sz);
	total_nb_pages = palloc_nb_free_page(&page_alloc);

	// Gestion de l'allocation pointeurs partagés
	sptra_init();
}

void kmem_bind_dynamic_slot(uint16_t num, phy_addr p_addr) {
	dslot_pt[num] = p_addr | PAGING_FLAG_W | PAGING_FLAG_P;
	invalide_page((uint_ptr)kmem_dynamic_slot(num));
}
uint16_t kmem_bind_dynamic_range(uint16_t num,
			phy_addr p_begin, phy_addr p_end) {
	for(phy_addr it = p_begin & PAGE_MASK; it < p_end; it += PAGE_SIZE)
		kmem_bind_dynamic_slot(num++, it);
	return num;
}

uint64_t* kmem_acc_pts_entry(uint_ptr v_addr, enum pgg_level rlvl, 
							 uint16_t flags) {
	kAssert(flags & PAGING_FLAG_W);
	uint64_t query_addr =
		(uint64_t) paging_acc_pml4(paging_get_lvl(pgg_pml4, v_addr));

	for(uint8_t lvl=4; lvl != rlvl;) { --lvl;
		uint64_t* query = (uint64_t*) query_addr;
		if(! ((*query) & PAGING_FLAG_P) ){
			klogf(Log_verb, "mem", "alloc for lvl %d", lvl);
			phy_addr new_struct = kmem_alloc_page();
			*query = new_struct | flags | PAGING_FLAG_P;

			query_addr = paging_rm_loop(query_addr);
			for(uint16_t i=0; i<512; ++i)
				((uint64_t*) query_addr)[i] = PAGING_FLAG_W;
		} else if((*query) & PAGING_FLAG_S)
			return NULL;
		else if (((*query) & flags) != flags) {
			*query |=  flags;
			query_addr = paging_rm_loop(query_addr);
			invalide_page(query_addr);
		} else
			query_addr = paging_rm_loop(query_addr);

		query_addr |= (v_addr >> (lvl * 9)) & PAGE_ENT_MASK;
	}

	return (uint64_t*) query_addr;
}

void kmem_print_paging(uint_ptr v_addr) {
	kprintf("paging to %p:\n", v_addr);
	uint64_t query_addr = (uint64_t) paging_acc_pml4(PML4_LOOP);
	for (uint8_t lvl=4; lvl; --lvl) {
		uint16_t rel = (v_addr >> (lvl * 9)) & PAGE_ENT_MASK;
		query_addr = paging_rm_loop(query_addr) | rel;
		uint64_t ent = *(uint64_t*)query_addr;
		kprintf("lvl %d: %x -> %p = %p\n",
				(int)lvl, (unsigned)(rel >> 3), query_addr, ent);
		if (! (ent & PAGING_FLAG_P) ) {
			kprintf("<not present>\n");
			return;
		}
	}
	kprintf("val: %p\n", *(uint64_t*)v_addr);
}

void kmem_print_info() {
	long unsigned nb_free = kmem_nb_page_free(),
		 		  nb_tot  = total_nb_pages,
				  nb_used = nb_tot - nb_free,
				  prop    = nb_used * 10000 / nb_tot;
	kprintf("free=%-7lu used=%-7lu total=%-6lu %3u.%02u%%\n",
			nb_free, nb_used, nb_tot,
			(unsigned)(prop / 100),
			(unsigned)(prop % 100));
}

uint8_t paging_map_to(uint_ptr v_addr, phy_addr p_addr,
		uint16_t flags, uint16_t p_flags) {
	uint64_t* query = kmem_acc_pts_entry(v_addr, pgg_pt, 
							flags | PAGING_FLAG_W);
	if (!query) return ~0;
	if ( (*query) & PAGING_FLAG_P) return 1; //Déjà assignée
	*query = p_addr | p_flags | PAGING_FLAG_P;
	return 0;
}

uint8_t kmem_paging_alloc(uint_ptr v_addr,
		uint16_t flags, uint16_t p_flags) {
	uint64_t* query = kmem_acc_pts_entry(v_addr, pgg_pt,
							flags | PAGING_FLAG_W);
	if (!query) return ~0;
	if ( (*query) & PAGING_FLAG_P) {
		*query |= p_flags;
		return 1;
	}
	*query = kmem_alloc_page() | p_flags | PAGING_FLAG_P;
	return 0;
}
uint8_t kmem_paging_alloc_rng(uint_ptr bg, uint_ptr ed,
		uint16_t flags, uint16_t p_flags) {
	for (uint_ptr it = bg & PAGE_MASK; it < ed; it += PAGE_SIZE) {
		uint8_t err = kmem_paging_alloc(it, flags, p_flags);
		if (err >= 2) return err;
	}
	return 0;
}

uint8_t paging_unmap(uint_ptr v_pg_addr) {
	//TODO: parents ?
	*paging_page_entry(v_pg_addr) = 0;
	return 0;
}
void kmem_paging_free(uint_ptr v_pg_addr) {
	phy_addr pg = *paging_page_entry(v_pg_addr) & PAGE_MASK;
	*paging_page_entry(v_pg_addr) = 0;
	kmem_free_page(pg);
}

void* kalloc_page() {
	uint_ptr v_addr = kheap_alloc_vpage();
	phy_addr p_addr = kmem_alloc_page();
	kassert(!paging_map_to(v_addr, p_addr, PAGING_FLAG_W, PAGING_FLAG_W),
			"kheap bind");
	return (void*)v_addr;
}

void  kfree_page(void* a) {
	uint_ptr v_addr = (uint_ptr) a;
	phy_addr p_addr = (*paging_page_entry(v_addr & PAGE_MASK)) & PAGE_MASK;
	kmem_free_page(p_addr);
	kheap_free_vpage(v_addr);
	paging_unmap(v_addr);
}

void kmem_init_pml4(uint64_t* pml4, phy_addr p_loc) {
	for(uint16_t i = 0; i < PAGE_ENT; ++i)
		pml4[i] = PAGING_FLAG_W;
	pml4[PML4_KERNEL_VIRT_ADDR] = *paging_acc_pml4(PML4_KERNEL_VIRT_ADDR)
		& (PAGE_MASK | PAGING_FLAG_P | PAGING_FLAG_W);
	pml4[PML4_LOOP] = p_loc | PAGING_FLAG_W | PAGING_FLAG_P;
}

void kmem_copy_pml4(uint64_t* pml4, phy_addr p_loc) {
	for(uint16_t i = 0; i < PAGE_ENT; ++i)
		pml4[i] = *paging_acc_pml4(i)
			& (PAGE_MASK | PAGING_FLAGS);
	pml4[PML4_LOOP] = p_loc | PAGING_FLAG_W | PAGING_FLAG_P;
}

void kmem_copy_rec(uint64_t* dst_entry, uint64_t* src_page, uint8_t lvl) {
	phy_addr pg_phy = kmem_alloc_page();
	*dst_entry = ((*dst_entry) & PAGE_OFS_MASK) | pg_phy;
	uint64_t* dst_page = (uint64_t*) paging_rm_loop((uint_ptr)dst_entry);
	if (lvl)
		for (uint16_t i = 0; i < PAGE_ENT; ++i) {
			dst_page[i] = src_page[i];
			if (dst_page[i] & PAGING_FLAG_P)
				kmem_copy_rec(
						dst_page+i,
						(uint64_t*) paging_rm_loop((uint_ptr)(src_page+i)),
						lvl - 1);
		}
	else
		for (uint16_t i = 0; i < PAGE_ENT; ++i)
			dst_page[i] = src_page[i];
}
void kmem_copy_paging(volatile phy_addr new_pml4) {
	kmem_bind_dynamic_slot(0, new_pml4);
	kmem_copy_pml4((uint64_t*)kmem_dynamic_slot(0), new_pml4);

	pml4_to_cr3(new_pml4);

	for (int i = 0; i < PML4_END_USPACE; ++i)
		if (*paging_acc_pml4(i) & PAGING_FLAG_P) {
			*paging_acc_pml4(PML4_COPY_RES)
				= *paging_acc_pml4(i) & ~PAGING_FLAG_U;
			paging_refresh();
			kmem_copy_rec(paging_acc_pml4(i),
					paging_acc_pdpt(PML4_COPY_RES,0), 3);
		}
}
