#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../def.h"

// --Paging--

//Page Map Level 4
extern uint32_t page_ml4[1024];

//Page Directory Pointer Table
extern uint32_t page_dpt_0[1024];
//Page Directory
extern uint32_t page_d_0[1024];
//Page Tables (x2)
extern uint32_t page_t_01[2*1024];

extern uint32_t page_dpt_1[1024];
extern uint32_t page_d_1[1024];
extern uint32_t page_t_23[2*1024];

/*
 * On met en place un paging de 4 niveaux permettant de couvrir
 * 4*9 + 12 = 48 bits d'adresse
 *
 * | PML4 | PDPT |  PD  |  PT  | page |
 * |47..39|38..30|29..21|20..12|11...0|
 *
 * On initialise 2 PT pour couvrir les adresses:
 * 0x00000000 - 0x003fffff
 * en les mappant aux adresses physiques identiques
 *
 * On initialise 2 PT pour couvrir les adresses virtuelles du kernel:
 * KVA - KVA + 3fffff
 * pour le kernel 64 bit
 *
 * TODO: erreur si bit 47 set
 */
void init_paging_directory(void){
	uint32_t *st0[5] = {
		page_d_0,   page_d_1,
		page_dpt_0, page_dpt_1,
		page_ml4
	};
	uint32_t *stf[2] = {
		page_t_01, page_t_23
	};
	uint32_t stf_l2[2] = {0, ((uint32_t)1)<<20};
	struct{size_t num;void* pa;} pml4_ent[3] = {
		{0,    page_dpt_0},
		{0x80, page_dpt_1},
		{0xff, page_ml4}
	};
	
	//initialisation à vide
	for(size_t s=0; s<5; ++s)
		for(uint16_t i=0; i < 1024; i+=2)
			st0[s][i] = 0x2;
	
	//initialisation des tables:
	//	0,0,0;     0,0,1;
	//	0x100,0,0; 0x100,0,0
	//couvrant chacune 512 pages de 4KB
	for(size_t s=0; s<2; ++s)
		for(uint32_t i_pte=0; i_pte<1024; ++i_pte) {
			stf[s][2*i_pte]     = ((i_pte << 12) + stf_l2[s])| 0x003;
			stf[s][2*i_pte + 1] = 0;
		}

#ifdef TEST_PAGING
	//mappe plusieurs adresses virtuelles vers la même adresse physique
	page_t_01[2] = (((uint32_t)0) << 12) | 0x003;
#endif

	for(size_t s=0; s<2; ++s) {
		//ajout des tables dans le directory
		st0[0|s][0] = ((uint32_t) stf[s]      ) | 0x003;
		st0[0|s][1] = 0;
		st0[0|s][2] = ((uint32_t)(stf[s]+1024)) | 0x003;
		st0[0|s][3] = 0;
		//ajout du directory dans la pdpt
		st0[2|s][0] = ((uint32_t)st0[0|s]    ) | 0x003;
		st0[2|s][1] = 0;
	}
	//ajout des pdpt dans le pml4
	// + accès aux structures depuis les adresses virtuelles
	for(size_t i=0; i<3; ++i){
		page_ml4[(pml4_ent[i].num<<1)|0] 
			= ((uint32_t)pml4_ent[i].pa) | 0x003;
		page_ml4[(pml4_ent[i].num<<1)|1] = 0;
	}
}

#ifdef TEST_PAGING
void test_paging_pre()
{
	/*adresses physiques*/
	*((uint8_t*)0x003fffff) = 42;
	*((uint8_t*)0x00400000) = 57;
	*((uint8_t*)0x00000001) = 42;
}
#endif
