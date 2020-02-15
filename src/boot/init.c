#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern uint32_t page_ml4[1024];
extern uint32_t page_dpt_0[1024];
extern uint32_t page_d_0[1024];
extern uint32_t page_t_01[2*1024];

/*
 * On initialise deux PT pour couvrir les adresses:
 * 0x00000 000 - 0x003ff fff
 * en les mappant aux adresses physiques identiques
 */
void init_paging_directory(void){
	//initialisation à vide
	for(uint16_t i=0; i < 1024; i+=2){
		page_ml4[i]   = 0x2;
		page_dpt_0[i] = 0x2;
		page_dpt_0[i+1] = 0;
	}
	
	//initialisation des tables 0,0,0 et 0,0,1
	//couvrant chacune 512 pages de 4KB
	for(uint32_t i_pte=0; i_pte<1024; ++i_pte){
		page_t_01[2*i_pte] = (i_pte << 12) | 0x003;
		page_t_01[2*i_pte + 1] = 0;
	}

#ifdef TEST_PAGING
	//mappe plusieurs adresses virtuelles vers la même adresse physique
	page_t_01[2] = (((uint32_t)0) << 12) | 0x003;
#endif
	
	//ajout des tables dans le directory
	page_d_0[0]   = ((uint32_t) page_t_01      ) | 0x003;
	page_d_0[1]   = 0;
	page_d_0[2]   = ((uint32_t)(page_t_01+1024)) | 0x003;
	page_d_0[3]   = 0;
	//ajout du directory dans la pdpt
	page_dpt_0[0] = ((uint32_t)page_d_0    ) | 0x003;
	page_dpt_0[1] = 0;
	//ajout de la pdpt dans le pml4
	page_ml4[0]   = ((uint32_t)page_dpt_0  ) | 0x003;
	page_ml4[1]   = 0;

	//TODO: accès au mapping depuis les adresses virtuelles
	//page_directory[1023] = ((uint32_t)page_directory) | 0x003;
}
