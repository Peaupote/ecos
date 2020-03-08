#ifndef _PAGE_ALLOC_H
#define _PAGE_ALLOC_H

#include<stddef.h>
#include<stdint.h>

//Gère un bloc de 2MB
//1: free
//0: pris
struct MemBlock {//Taille multiple de 8 octets
	uint16_t nb_at_lvl[4];
	uint64_t lvl_2_0;      //8 * 8
	uint32_t lvl_2_21;     //8 * (1+3)
	uint32_t lvl_1_10[8];  //8 * (1+3)
	uint8_t  lvl_0_0 [64]; //8 * 1
} __attribute__((packed));

void mblock_init(struct MemBlock* b);

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

#endif
