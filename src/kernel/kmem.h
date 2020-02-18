#include <stdint.h>

#include "../def.h"

#define PAGE_SIZE 			0x1000
#define PAGE_MASK 			((uint64_t) ~0x111)
#define PAGE_OFS_MASK		((uint64_t)  0x111)

extern uint32_t end_kernel;

typedef uint64_t phy_addr;//adresse physique
typedef uint64_t uint_ptr;//adresse virtuelle

static inline uint_ptr paging_set_pml4(uint16_t e) {
	return ((uint_ptr) e) << 39;
}
static inline uint_ptr paging_set_pdpt(uint16_t e) {
	return ((uint_ptr) e) << 30;
}
static inline uint_ptr paging_set_pd  (uint16_t e) {
	return ((uint_ptr) e) << 21;
}
static inline uint_ptr paging_set_pt  (uint16_t e) {
	return ((uint_ptr) e) << 12;
}

static inline uint64_t* paging_pts_acc(uint16_t j0, uint16_t j1,
		uint16_t j2, uint16_t j3, uint16_t num) {
	return (uint64_t*)(
		paging_set_pml4(j0)
	  | paging_set_pdpt(j1)
	  | paging_set_pd  (j2)
	  | paging_set_pt  (j3)
	  | (((uint_ptr)num)<<3) );
}

static inline uint64_t* paging_acc_pml4(uint16_t pml4_e) {
	return paging_pts_acc(
			PML4_LOOP, PML4_LOOP, PML4_LOOP, PML4_LOOP, pml4_e);
}
static inline uint64_t* paging_acc_pdpt(uint16_t pml4_e, uint16_t pdpt_e) {
	return paging_pts_acc(
			PML4_LOOP, PML4_LOOP, PML4_LOOP, pml4_e, pdpt_e);
}
static inline uint64_t* paging_acc_pd(uint16_t pml4_e, uint16_t pdpt_e,
		uint16_t pd_e) {
	return paging_pts_acc(
			PML4_LOOP, PML4_LOOP, pml4_e, pdpt_e, pd_e);
}
static inline uint64_t* paging_acc_pt(uint16_t pml4_e, uint16_t pdpt_e,
		uint16_t pd_e, uint16_t pt_e) {
	return paging_pts_acc(
			PML4_LOOP, pml4_e, pdpt_e, pd_e, pt_e);
}

static inline uint64_t* paging_page_entry(uint_ptr page_v_addr) {
	return (uint64_t*)(
		(((uint_ptr) PML4_LOOP)<<39)
		| (page_v_addr>>9));
}

static inline phy_addr paging_phy_addr_page(uint_ptr page_v_addr) {
	return (*paging_page_entry(page_v_addr)) & PAGE_MASK;
}
static inline phy_addr paging_phy_addr(uint_ptr v_addr) {
	return paging_phy_addr_page(v_addr & PAGE_MASK)
		| (v_addr & PAGE_OFS_MASK);
}

void kmem_init();
uint_ptr kmem_alloc_page();
