#ifndef _PAGE_ALLOC_H
#define _PAGE_ALLOC_H

#include<stddef.h>
#include<stdint.h>

#ifndef __is_test_unit
#include "../../util/paging.h"
#endif

#define MBLOC_SIZE     0x200000
#define MBLOC_MASK     (~(uint64_t) 0x1fffff)
#define MBLOC_OFS_MASK ( (uint64_t) 0x1fffff)
#define MBLOC_SHIFT    21

//Gère un bloc de 2MB
//1: free
//0: pris
struct MemBlock {//Taille multiple de 8 octets
	uint16_t nb_at_lvl[4];
	uint64_t lvl_2_0;      //8 * 8
	uint32_t lvl_2_21;     //8 * (1+3)
	uint32_t lvl_1_10[8];  //8 * (1+3)
	uint8_t  lvl_0_0 [64]; //8 * 1
};

//Arbre d'arité 64, la racine est au bit 63
struct MemBlockTree {
	size_t    height;
	size_t    lvs; //indice de la première feuille
	size_t    lvs_lim;
	uint64_t *cnt;
};

struct PageAllocator {
	struct MemBlock*    mblocks;
	struct MemBlockTree mbt_part;
	struct MemBlockTree mbt_full;
	size_t   nb_blocks;
	phy_addr addr_begin;
	phy_addr addr_end;
};

//Créé un bloc entièrement disponible
void mblock_init1(struct MemBlock* b);
//Créé un bloc entièrement non disponible
void mblock_init0(struct MemBlock* b);

static inline uint16_t mblock_page_size_lvl(uint8_t lvl) {
	return 1 << (3 * lvl);
}

//Un sous-block du niveau demandé doit être libre
//Retourne l'adresse relative en nombre de pages
typedef uint16_t (*mblock_alloc_f)(struct MemBlock*);
uint16_t mblock_alloc_lvl_0(struct MemBlock* b);
uint16_t mblock_alloc_lvl_1(struct MemBlock* b);
uint16_t mblock_alloc_lvl_2(struct MemBlock* b);
void     mblock_alloc_lvl_3(struct MemBlock* b);
extern mblock_alloc_f mblock_alloc[4];

//On ne supose pas que n est la première adresse du sous-bloc
typedef void (*mblock_free_f)(struct MemBlock*, uint16_t);
void   mblock_free_lvl_0(struct MemBlock* b, uint16_t n);
void   mblock_free_lvl_1(struct MemBlock* b, uint16_t n);
void   mblock_free_lvl_2(struct MemBlock* b, uint16_t n);
void   mblock_free_lvl_3(struct MemBlock* b);
extern mblock_free_f mblock_free[4];

//keep > 0, le bloc doit être alloué
//conserve les `keep` premiers sous-blocs
typedef void (*mblock_split_f)(struct MemBlock*, uint16_t, uint8_t);
void   mblock_split_lvl_1(struct MemBlock* b, uint16_t n, uint8_t keep);
void   mblock_split_lvl_2(struct MemBlock* b, uint16_t n, uint8_t keep);
void   mblock_split_lvl_3(struct MemBlock* b, uint8_t keep);
extern mblock_split_f mblock_split[3];

uint8_t  mblock_non_empty(struct MemBlock* b);
uint8_t  mblock_full_free(struct MemBlock* b);
size_t   mblock_nb_page_free(struct MemBlock* b);

void mblock_free_rng(struct MemBlock* b, uint16_t begin, uint16_t end);

//Le bloc ne doit pas être completement alloué
uint16_t mblock_alloc_page(struct MemBlock* b);


size_t  mbtree_height_for(size_t nb_blocks);
size_t  mbtree_intn_for(size_t h);
//Espace nécessaire (byte), multiple de 8
size_t  mbtree_space_for(size_t intn, size_t nb_blocks);
//Créé un arbre vide
void    mbtree_init(struct MemBlockTree* t, size_t height, size_t intn,
					size_t space, uint64_t* cnt);
void    mbtree_add(struct MemBlockTree* t, size_t i);
void    mbtree_rem(struct MemBlockTree* t, size_t i);
uint8_t mbtree_get(struct MemBlockTree* t, size_t i);
uint8_t mbtree_non_empty(struct MemBlockTree* t);
size_t  mbtree_find(struct MemBlockTree* t);
void    mbtree_add_rng(struct MemBlockTree* t, size_t begin, size_t end);


void palloc_init(struct PageAllocator*, struct MemBlock* mbs,
		size_t height, size_t intn, size_t space,
		uint64_t* cnt0, uint64_t* cnt1);
void    palloc_add_zones(struct PageAllocator*,
		uint64_t lims[], size_t lims_sz);
void    sort_limits(uint64_t* a, size_t sz);

phy_addr palloc_alloc_page(struct PageAllocator*);
void     palloc_free_page (struct PageAllocator*, phy_addr);
size_t   palloc_nb_free_page(struct PageAllocator*);

#endif
