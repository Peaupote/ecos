#ifndef _PAGING_H
#define _PAGING_H

/*
 * Paging de 4 niveaux permettant de couvrir
 * 4*9 + 12 = 48 bits d'adresse
 *
 * | PML4 | PDPT |  PD  |  PT  | page |
 * |47..39|38..30|29..21|20..12|11...0|
 *
 * L'entrée PML4_LOOP boucle et permet d'accéder aux structures de paging
 * depuis les adresses virtuelles
 */

#define PAGE_SIZE 			0x1000
#define PAGE_MASK 			(~(uint64_t) 0xfff)
#define PAGE_OFS_MASK		( (uint64_t) 0xfff)

#define VADDR_MASK ((((uint64_t)1)<<48)-1)

#define PML4_LOOP         	0xff

//Present
#define PAGING_FLAG_P		0x1
//Read/write: read only si 0
#define PAGING_FLAG_R		0x2
//User/supervisor
#define PAGING_FLAG_U		0x4
//page Size: si 1 dans le PD utilise une page de 2MB (si PAE)
//à la place d'une PT requiert PSE (ou PAE)
#define PAGING_FLAG_S		(1<<7)

#define PAGING_FLAGS PAGING_FLAG_P|PAGING_FLAG_R|PAGING_FLAG_U|PAGING_FLAG_S

typedef uint64_t phy_addr;//adresse physique
typedef uint64_t uint_ptr;//adresse virtuelle

static inline uint_ptr paging_set_pml4(uint16_t e) {
	return ((uint_ptr) e) << 39;
}
static inline uint16_t paging_get_pml4(uint_ptr v) {
	return (uint16_t)((v >> 39) & 0x1ff);
}
static inline uint_ptr paging_set_pdpt(uint16_t e) {
	return ((uint_ptr) e) << 30;
}
static inline uint16_t paging_get_pdpt(uint_ptr v) {
	return (uint16_t)((v >> 30) & 0x1ff);
}
static inline uint_ptr paging_set_pd  (uint16_t e) {
	return ((uint_ptr) e) << 21;
}
static inline uint16_t paging_get_pd  (uint_ptr v) {
	return (uint16_t)((v >> 21) & 0x1ff);
}
static inline uint_ptr paging_set_pt  (uint16_t e) {
	return ((uint_ptr) e) << 12;
}
static inline uint16_t paging_get_pt  (uint_ptr v) {
	return (uint16_t)((v >> 12) & 0x1ff);
}

static inline uint_ptr paging_rm_loop (uint_ptr v) {
	return (v << 9) & VADDR_MASK;
}

#ifndef __i386__
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

#endif

static inline void paging_refresh() {
	asm volatile("movq %%cr3,%%rax; movq %%rax,%%cr3;" : : : "memory");
}

static inline void pml4_to_cr3(phy_addr pml4_loc) {
	asm volatile("movq %%rax,%%cr3" :: "rax"(pml4_loc) : "memory");
}

#endif

