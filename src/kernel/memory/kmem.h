#ifndef _KMEM_H
#define _KMEM_H

#include <stdint.h>

#include "../../def.h"
#include "../../util/paging.h"

#define KERNEL_PDPT_LADDR 0x100
#define KERNEL_PDPT_DSLOT 0x180
#define KERNEL_PDPT_HEAP  0x181
#define KERNEL_PDPT_SPGI  0x182
#define PML4_COPY_RES     0xfe
#define PML4_END_USPACE   PML4_KERNEL_VIRT_ADDR

#include "../kutil.h"
#include "page_alloc.h"

extern phy_addr kernel_pml4;
extern uint32_t bgn_kernel;
extern uint32_t end_kernel;

extern uint8_t  low_addr[];

extern struct PageAllocator page_alloc;
extern struct MemBlockTree  khep_alloc;

void     kmem_init_paging();
void     kmem_init_alloc(uint32_t boot_info);

static inline uint64_t align_to(uint64_t addr, uint64_t align) {
	return (addr % align)
		? addr + align - (addr % align)
	    : addr;
}

static inline void* kmem_dynamic_slot(uint16_t num) {
	return paging_pts_acc(PML4_LOOP, PML4_KERNEL_VIRT_ADDR,
			KERNEL_PDPT_DSLOT, num, 0);
}
static inline void* kmem_dynamic_slot_at(uint16_t num, phy_addr p) {
	return paging_adr_acc(PML4_LOOP, PML4_KERNEL_VIRT_ADDR,
			KERNEL_PDPT_DSLOT, num, p & PAGE_OFS_MASK);
}
void     kmem_bind_dynamic_slot(uint16_t num, phy_addr p_addr);
//Retourne le prochain slot libre
uint16_t kmem_bind_dynamic_range(uint16_t num,
			phy_addr p_begin, phy_addr p_end);

static inline phy_addr kmem_alloc_page() {
	return palloc_alloc_page(&page_alloc);
}
static inline void kmem_free_page(phy_addr p) {
	palloc_free_page(&page_alloc, p);
}
static inline size_t kmem_nb_page_free() {
	return palloc_nb_free_page(&page_alloc);
}
static inline uint_ptr kheap_alloc_vpage() {
	kassert(mbtree_non_empty(&khep_alloc), "kheap full");
	size_t pg = mbtree_find(&khep_alloc);
	mbtree_rem(&khep_alloc, pg);
	return (uint_ptr)paging_adr_acc(PML4_KERNEL_VIRT_ADDR, KERNEL_PDPT_HEAP,
			0, pg, 0);
}
static inline void kheap_free_vpage(uint_ptr a) {
	size_t pg = (a - (uint_ptr)paging_adr_acc(
			PML4_KERNEL_VIRT_ADDR, KERNEL_PDPT_HEAP, 0, 0, 0))
			>> PAGE_SHIFT;
	mbtree_add(&khep_alloc, pg);
}

void* kalloc_page();
void  kfree_page(void* v_addr);

uint64_t* kmem_acc_pts_entry(uint_ptr v_addr, uint8_t rlvl, uint16_t flags);

void kmem_print_paging(uint_ptr v_addr);

//retourne:
//	0   si la page a bien été affectée
//	1   si l'emplacement est déjà pris
//	~0	en cas d'erreur
uint8_t paging_map_to(uint_ptr v_addr, phy_addr p_addr);
//alloue une page si non déjà affecté
uint8_t kmem_paging_alloc(uint_ptr v_addr, uint16_t flags, uint16_t p_flags);

uint8_t paging_unmap(uint_ptr v_pg_addr);
void    kmem_paging_free(uint_ptr v_pg_addr);

void kmem_init_pml4(uint64_t* addr, phy_addr loc);

//Crée de nouvelles structures de paging en copiant la mémoire du processus
//à partir du paging actuel et switch sur le nouveau paging
void kmem_copy_paging(phy_addr new_pml4);

//définies dans ../idt_as.S
extern void pml4_to_cr3(phy_addr pml4_loc);
extern void invalide_page(uint_ptr page_v_addr);
extern void paging_refresh();

#endif
