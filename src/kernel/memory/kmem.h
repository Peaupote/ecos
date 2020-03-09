#ifndef _KMEM_H
#define _KMEM_H

#include <stdint.h>

#include "../../util/paging.h"

#define KERNEL_PDPT_LADDR 0x100
#define PML4_COPY_RES      0xfe
#define PML4_MAX_USPACE   (PML4_KERNEL_VIRT_ADDR-1)

extern uint32_t end_kernel;

extern uint8_t  low_addr[];
extern uint8_t  dynamic_slot[];

void     kmem_init();
phy_addr kmem_alloc_page();
void     kmem_free_page(phy_addr);

//retourne:
//	0   si la page a bien été affectée
//	1   si l'emplacement est déjà pris
//	~0	en cas d'erreur
uint8_t paging_map_to(uint_ptr v_addr, phy_addr p_addr);
//affecte même si l'emplacement est déjà pris
uint8_t paging_force_map_to(uint_ptr v_addr, phy_addr p_addr);
//alloue une page si non déjà affecté
uint8_t kmem_paging_alloc(uint_ptr v_addr, uint16_t flags);

void kmem_init_pml4(uint64_t* addr, phy_addr loc);

//Crée de nouvelles structures de paging en copiant la mémoire du processus
//à partir du paging actuel et switch sur le nouveau paging
void kmem_copy_paging(phy_addr new_pml4);

extern void pml4_to_cr3(phy_addr pml4_loc);

#endif
