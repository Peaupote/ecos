/*
 * Gestion des entrées de paging affectées lors de l'accès:
 * - pages partagées, copiées lors de l'accès si plusieurs utilisateurs
 * - nouvelle page, crées lors de l'accès
 */
#ifndef _SHARED_PG_H
#define _SHARED_PG_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <util/paging.h>

//Present: set si une page est enregistrée
#define SPAGING_FLAG_P   PAGING_FLAG_Y1
//Value: set si référence une page partagée
#define SPAGING_FLAG_V   PAGING_FLAG_Y2
#define SPAGING_FLAGS_0 (SPAGING_FLAG_P|SPAGING_FLAG_V)
#define SPAGING_FLAGS_1 (PAGING_FLAG_W|PAGING_FLAG_U|PAGING_FLAG_S|PAGING_FLAG_G) 
//numéro du spg_info
#define SPAGING_INFO_SHIFT 11

uint8_t handle_PF(uint_ptr fault_vaddr);
//Prépare un nouveau paging avec copie des pages lors de l'accès
//Reste sur l'ancien paging
void    kmem_fork_paging(phy_addr new_pml4);
//Le paging actuel doit être `old_pml4`, libère les structures de paging et
//les pages référencées et switch sur `new_pml4`
void    kmem_free_paging(phy_addr old_pml4, phy_addr new_pml4);
void    kmem_free_paging_range(uint64_t* page_bg,
				enum pgg_level lvl, uint16_t lim);

#endif
