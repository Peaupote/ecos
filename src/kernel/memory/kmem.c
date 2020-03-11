#include "kmem.h"

#include<stddef.h>

#include "../../def.h"
#include "page_alloc.h"
#include "../kutil.h"
#include "../../util/multiboot.h"

#include "../tty.h"
#include "../../util/vga.h"

// Accès aux 2MB d'adresses basses
uint64_t laddr_pd[512] __attribute__ ((aligned (PAGE_SIZE)));

// Emplacements dynamiques
uint64_t dslot_pt[512] __attribute__ ((aligned (PAGE_SIZE)));

// Heap: stocke les structures du kernel de taille déterminiée à l'éxécution
uint64_t heap_pd[512] __attribute__ ((aligned (PAGE_SIZE)));
uint64_t heap_pt0[512] __attribute__ ((aligned (PAGE_SIZE)));
size_t   heap_next_page;

struct PageAllocator page_alloc;


void kmem_init_paging() {
	for(uint16_t i=0; i<512; ++i)
		laddr_pd[i] = heap_pd[i] = heap_pt0[i] = dslot_pt[i]
			= PAGING_FLAG_R;
	laddr_pd[0] = 0 | PAGING_FLAG_S | PAGING_FLAG_R | PAGING_FLAG_P;
	heap_pd [0] = paging_phy_addr_page((uint_ptr)heap_pt0)
                | PAGING_FLAG_R | PAGING_FLAG_P;
	*paging_acc_pdpt(PML4_KERNEL_VIRT_ADDR, KERNEL_PDPT_LADDR)
		= paging_phy_addr_page((uint_ptr) laddr_pd)
		| PAGING_FLAG_R | PAGING_FLAG_P;
	*paging_acc_pdpt(PML4_KERNEL_VIRT_ADDR, KERNEL_PDPT_DSLOT)
		= paging_phy_addr_page((uint_ptr) dslot_pt)
		| PAGING_FLAG_R | PAGING_FLAG_P;
	*paging_acc_pdpt(PML4_KERNEL_VIRT_ADDR, KERNEL_PDPT_HEAP)
		= paging_phy_addr_page((uint_ptr) heap_pd)
		| PAGING_FLAG_R | PAGING_FLAG_P;
}


void kmem_init_alloc(uint32_t boot_info) {
	//On récupère la carte de la mémoire fournie par GRUB
	kmem_bind_dynamic_range(0, 
			boot_info, boot_info + sizeof(struct multiboot_header));
	multiboot_info_t* mbh = (multiboot_info_t*)
			kmem_dynamic_slot_at(0, boot_info);
	if(! (mbh->flags & MULTIBOOT_INFO_MEM_MAP)) kpanic("mem map");

	phy_addr mmap_addr = mbh->mmap_addr;
	size_t mmap_length = mbh->mmap_length;
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
	size_t s_space = page_alloc.nb_blocks * sizeof(struct MemBlock);
	size_t h       = mbtree_height_for(page_alloc.nb_blocks);
	size_t intn    = mbtree_intn_for(h);
	size_t t_spac1 = mbtree_space_for(intn, page_alloc.nb_blocks);
	size_t t_space = 2 * t_spac1;
	size_t t_space_bg = align_to(s_space, 8);
	s_space = t_space_bg + t_space;

	if (s_space > 512 * PAGE_SIZE) kpanic("s_space");

	// On cherche un emplacement pour stocker les structures
	phy_addr ek_pg = align_to((phy_addr) end_kernel, PAGE_SIZE);
	phy_addr s_pya = (ek_pg + s_space > mmap_addr)
		? align_to(mmap_addr + mmap_length, PAGE_SIZE)
		: ek_pg;

	// On map cet emplacement sur le tas
	for (heap_next_page = 0; heap_next_page * PAGE_SIZE < s_space;
			++heap_next_page)
		heap_pt0[heap_next_page] = (s_pya + heap_next_page * PAGE_SIZE)
			    | PAGING_FLAG_R | PAGING_FLAG_P | PAGING_FLAG_G;
	uint8_t* s_space_bg = (uint8_t*) paging_pts_acc
				(PML4_KERNEL_VIRT_ADDR, KERNEL_PDPT_HEAP, 0, 0, 0);
	
	palloc_init(&page_alloc,
		(struct MemBlock*) s_space_bg, h, intn, t_spac1,
		(uint64_t*)(s_space_bg + t_space_bg),
		(uint64_t*)(s_space_bg + t_space_bg + t_spac1));

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
}

void kmem_bind_dynamic_slot(uint16_t num, phy_addr p_addr) {
	dslot_pt[num] = p_addr | PAGING_FLAG_R | PAGING_FLAG_P;
	invalide_page((uint_ptr)kmem_dynamic_slot(num));
}
uint16_t kmem_bind_dynamic_range(uint16_t num,
			phy_addr p_begin, phy_addr p_end) {
	for(phy_addr it = p_begin & PAGE_MASK; it < p_end; it += PAGE_SIZE)
		kmem_bind_dynamic_slot(num++, it);
	return num;
}

phy_addr kmem_alloc_page() {
	return palloc_alloc_page(&page_alloc);
}
void kmem_free_page(phy_addr p) {
	palloc_free_page(&page_alloc, p);
}
size_t kmem_nb_page_free() {
	return palloc_nb_free_page(&page_alloc);
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
	kmem_bind_dynamic_slot(0, new_pml4);
	kmem_copy_pml4((uint64_t*)kmem_dynamic_slot(0), new_pml4);

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
