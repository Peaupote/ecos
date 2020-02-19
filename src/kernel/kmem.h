#ifndef _KMEM_H
#define _KMEM_H

#include <stdint.h>

#include "../util/paging.h"

extern uint32_t end_kernel;

void kmem_init();
uint_ptr kmem_alloc_page();

//retourne:
//	0 	si la page a bien été affectée
//	1 	si l'emplacement est déjà pris
//	~0	en cas d'erreur
uint8_t paging_map_to(uint_ptr v_addr, phy_addr p_addr);

#endif
