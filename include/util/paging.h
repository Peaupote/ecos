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

#define PAGE_SIZE           0x1000
#define PAGE_MASK           (~(uint64_t) 0xfff)
#define PAGE_OFS_MASK		( (uint64_t) 0xfff)
#define PAGE_ENT_MASK		( (uint64_t) 0xff8)
#define PAGE_IENT_MASK		0x1ff
#define PAGE_ENT            (PAGE_SIZE / 8)
#define PAGE_SHIFT          12

#define VADDR_MASK          ((((uint64_t)1)<<48)-1)
#define PGG_LVL_SHIFT       9

#define PML4_LOOP           0xff

//Present
#define PAGING_FLAG_P		(1<<0)
//Read/write: read only si 0
#define PAGING_FLAG_W		(1<<1)
//User/supervisor
#define PAGING_FLAG_U		(1<<2)
//page Size: si 1 dans le PD utilise une page de 2MB (si PAE)
//à la place d'une PT requiert PSE (ou PAE)
#define PAGING_FLAG_S		(1<<7)
//désactive la mise à jour du TLB lors d'un changement de CR3
//requiert PGE
#define PAGING_FLAG_G		(1<<8)
//ignorés: peuvent être utilisés par le système
#define PAGING_FLAG_Y1      (1<<9)
#define PAGING_FLAG_Y2      (1<<10)
#define PAGING_FLAG_Y3      (1<<11)

#define PAGING_FLAGS PAGING_FLAG_P|PAGING_FLAG_W|PAGING_FLAG_U|PAGING_FLAG_S|PAGING_FLAG_G

typedef uint64_t phy_addr;//adresse physique
typedef uint64_t uint_ptr;//adresse virtuelle

enum pgg_level {
	pgg_page = 0,
	pgg_pt   = 1,
	pgg_pd   = 2,
	pgg_pdpt = 3,
	pgg_pml4 = 4
};

static inline uint_ptr paging_lvl_shift(enum pgg_level lvl) {
	return (lvl - 1) * PGG_LVL_SHIFT + PAGE_SHIFT;
}

static inline uint16_t paging_get_lvl(enum pgg_level lvl, uint_ptr v) {
	return (v >> paging_lvl_shift(lvl)) & PAGE_IENT_MASK;
}

static inline uint_ptr paging_set_lvl(enum pgg_level lvl, uint16_t ient) {
	return ((uint_ptr) ient) << paging_lvl_shift(lvl);
}

static inline uint_ptr paging_add_lvl(enum pgg_level lvl, uint_ptr ient) {
	return ient << paging_lvl_shift(lvl);
}

static inline uint_ptr paging_rm_loop (uint_ptr v) {
    return (v << 9) & VADDR_MASK;
}
static inline uint_ptr paging_add_loop (uint_ptr v) {
	return paging_set_lvl(pgg_pml4, PML4_LOOP) | (v >> PGG_LVL_SHIFT);
}

#ifndef __i386__
static inline uint64_t* paging_pts_acc(uint16_t j0, uint16_t j1,
        uint16_t j2, uint16_t j3, uint16_t num) {
    return (uint64_t*)(
        paging_set_lvl(4, j0)
      | paging_set_lvl(3, j1)
      | paging_set_lvl(2, j2)
      | paging_set_lvl(1, j3)
      | paging_set_lvl(0, num) );
}
static inline uint64_t* paging_adr_acc(uint16_t j0, uint_ptr j1,
        uint_ptr j2, uint_ptr j3, uint_ptr ofs) {
    return (uint64_t*)(
        paging_set_lvl(pgg_pml4, j0)
      + paging_add_lvl(3, j1)
      + paging_add_lvl(2, j2)
      + paging_add_lvl(1, j3)
      + ofs );
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
    return (uint64_t*) paging_add_loop(page_v_addr);
}

static inline phy_addr paging_phy_addr_page(uint_ptr page_v_addr) {
    return (*paging_page_entry(page_v_addr)) & PAGE_MASK;
}
static inline phy_addr paging_phy_addr(uint_ptr v_addr) {
    return paging_phy_addr_page(v_addr & PAGE_MASK)
        | (v_addr & PAGE_OFS_MASK);
}

#endif

#endif
