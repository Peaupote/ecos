#ifndef _PAGE_ALLOC_H
#define _PAGE_ALLOC_H

#include<stddef.h>
#include<stdint.h>

#ifndef TEST_UNIT
#include "kmem.h"
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
	phy_addr addr;
};

//Arbre d'arité 64, la racine est au bit 63
struct MemBlockTree {
	size_t    height;
	size_t    lvs; //indice de la première feuille
	size_t    lvs_lim;
	uint64_t *cnt;
};

//Créé un bloc entièrement disponible
void mblock_init(struct MemBlock* b, phy_addr a);

//Un sous-block du niveau demandé doit être libre
//Retourne l'adresse relative en nombre de pages
uint16_t mblock_alloc_lvl_0(struct MemBlock* b);
uint16_t mblock_alloc_lvl_1(struct MemBlock* b);
uint16_t mblock_alloc_lvl_2(struct MemBlock* b);
void     mblock_alloc_lvl_3(struct MemBlock* b);


//On ne supose pas que n est la première adresse du sous-bloc
void mblock_free_lvl_0(struct MemBlock* b, uint16_t n);
void mblock_free_lvl_1(struct MemBlock* b, uint16_t n);
void mblock_free_lvl_2(struct MemBlock* b, uint16_t n);
void mblock_free_lvl_3(struct MemBlock* b);

//keep > 0, le bloc doit être alloué
//conserve les `keep` premiers sous-blocs
void mblock_split_lvl_1(struct MemBlock* b, uint16_t n, uint8_t keep);
void mblock_split_lvl_2(struct MemBlock* b, uint16_t n, uint8_t keep);
void mblock_split_lvl_3(struct MemBlock* b, uint8_t keep);

//Le bloc ne doit pas être completement alloué
uint16_t mblock_alloc_page(struct MemBlock* b);


size_t  mbtree_height_for(size_t nb_blocks);
size_t  mbtree_intn_for(size_t h);
size_t  mbtree_space_for(size_t intn, size_t nb_blocks);
//Créé un arbre vide
void    mbtree_init(struct MemBlockTree* t, size_t height, size_t intn,
					size_t space, uint64_t* cnt);
void    mbtree_add(struct MemBlockTree* t, size_t i);
void    mbtree_rem(struct MemBlockTree* t, size_t i);
uint8_t mbtree_get(struct MemBlockTree* t, size_t i);
uint8_t mbtree_non_empty(struct MemBlockTree* t);
size_t  mbtree_find(struct MemBlockTree* t);

#endif
