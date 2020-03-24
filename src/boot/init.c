#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <util/paging.h>
#include <def.h>

#define F_PRS	PAGING_FLAG_W | PAGING_FLAG_P

// --Paging--

//Page Map Level 4
extern uint32_t page_ml4[1024];

//Page Directory Pointer Table
extern uint32_t page_dpt_0[1024];
//Page Directory
extern uint32_t page_d_0[1024];

extern uint32_t page_dpt_1[1024];
extern uint32_t page_d_1[1024];
//Page Tables (x2)
extern uint32_t page_t_23[2*1024];

extern uint8_t KPA;

/*
 * On initialise 2 pages de 2MB pour couvrir les adresses:
 * 0x00000000 -- 0x003fffff
 * en les mappant aux adresses physiques identiques
 *
 * On initialise 2 pages de 2MB pour couvrir les adresses virtuelles du kernel:
 * KVA -- KVA + 0x3fffff
 *
 * TODO: erreur si bit 47 set
 */
void init_paging_directory(void){
    uint32_t *st0[5] = {
        page_d_0,   page_d_1,
        page_dpt_0, page_dpt_1,
        page_ml4
    };
    struct{size_t num;void* pa;} pml4_ent[3] = {
        {0,    page_dpt_0},
        {PML4_KERNEL_VIRT_ADDR, page_dpt_1},
        {PML4_LOOP, page_ml4}
    };

    //initialisation à vide
    for(size_t s=0; s<5; ++s)
        for(uint16_t i=0; i < 1024; i+=2)
            st0[s][i] = PAGING_FLAG_W;

    //initialisation des tables:
    //	PML4_KERNEL_VIRT_ADDR,0,0; PML4_KERNEL_VIRT_ADDR,0,1
    //couvrant chacune 512 pages de 4KB
    for(uint32_t i_pte=0; i_pte<1024; ++i_pte) {
        page_t_23[2*i_pte]     = ((i_pte << 12) + (uint32_t)(&KPA))
                               | F_PRS | PAGING_FLAG_G;
        page_t_23[2*i_pte + 1] = 0;
    }

    //Mapping identité: ajout des pages de 2MB dans le directory
    page_d_0[0] = PAGING_FLAG_S | F_PRS;
    page_d_0[1] = 0;
    page_d_0[2] = (1<<21) | PAGING_FLAG_S | F_PRS;
    page_d_0[3] = 0;

    //Kernel: ajout des 2 tables dans le directory
    page_d_1[0] = ((uint32_t) page_t_23      ) | F_PRS;
    page_d_1[1] = 0;
    page_d_1[2] = ((uint32_t)(page_t_23+1024)) | F_PRS;
    page_d_1[3] = 0;

    for(size_t s=0; s<2; ++s) {
        //ajout des directories dans les pdpt
        st0[2|s][0] = ((uint32_t)st0[0|s]     ) | F_PRS;
        st0[2|s][1] = 0;
    }
    //ajout des pdpt dans le pml4
    // + accès aux structures depuis les adresses virtuelles
    for(size_t i=0; i<3; ++i){
        page_ml4[(pml4_ent[i].num<<1)|0]
            = ((uint32_t)pml4_ent[i].pa) | F_PRS;
        page_ml4[(pml4_ent[i].num<<1)|1] = 0;
    }
}
