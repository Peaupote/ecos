#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern uint32_t page_directory[1024];
extern uint32_t page_table_0[1024];

/*
 * On initialise la PT 0 entièrement pour couvrir les adresses:
 * 0x00000 000 - 0x003ff fff
 * en les mappant aux adresses physiques identiques
 */
void init_paging_directory(void){
	//initialisation du directory
	for(uint32_t *pde=page_directory+1; pde<page_directory+1024; ++pde)
		*pde = 0x00000002; //pas de page table
	
	//initialisation de la table 0
	for(uint32_t i_pte=0; i_pte<1024; ++i_pte)
		page_table_0[i_pte] = (i_pte << 12) | 0x003;
	
	//mappe plusieurs adresses virtuelles vers la même adresse physique
	page_table_0[1] = (((uint32_t)0) << 12) | 0x003;
	
	//ajout de la table 0 dans le directory
	page_directory[0] = ((uint32_t)page_table_0) | 0x003;

	//accès au mapping depuis les adresses virtuelles
	page_directory[1023] = ((uint32_t)page_directory) | 0x003;
}
