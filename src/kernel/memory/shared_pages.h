#ifndef _SHARED_PG_H
#define _SHARED_PG_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

//Present: set si une page est enregistrée
#define SPAGING_FLAG_P (1<<5)
//Value: set si référence une page partagée
#define SPAGING_FLAG_V (1<<6)
#define SPAGING_FLAGS  (SPAGING_FLAG_P|SPAGING_FLAG_V)
//numéro du spg_info
#define SPAGING_INFO_SHIFT 11

#include "../../util/paging.h"

uint8_t handle_PF(uint_ptr fault_vaddr);
//Prépare un nouveau paging avec copie des pages lors de l'accès
//Reste sur l'ancien paging
void    kmem_fork_paging(phy_addr new_pml4);
//Le paging actuel doit être `old_pml4`, libère les structures de paging et
//les pages référencées et switch sur `new_pml4`
void kmem_free_paging(phy_addr old_pml4, phy_addr new_pml4);

#endif
