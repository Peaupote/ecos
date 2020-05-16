/*
 * Allocation de pointeurs partagés (addresse + nombre d'utilisateurs)
 */
#ifndef _SHARED_PTR_H
#define _SHARED_PTR_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <util/paging.h>

struct sptr_hd {
	phy_addr addr;
	uint64_t count;
};

/* Nœud de l'arbre binaire de recherche utilisé pour référencer les
   entrées non utilisées, l'indices des entrées sert de clef ce qui permet
   d'accéder à l'indice le plus faible (allocation) et à l'indice le plus
   élevé (libération de la mémoire)
   Chaque indice a une place fixe dans l'arbre (mais peut remonter s'il
   manque des nœuds intermédiaires) qui est déséquilibré pour indexer un
   nombre arbitrairement grand d'entrée.
  
      1    3    7
   -> . -- . -- . ...
           |    |
           |    |
           .2   .5
                | \
                |  \
                .4  .6
*/
typedef struct {
	uint64_t left;
	uint64_t right;
} sptra_node;

union sptra_entry{
	struct sptr_hd info;
	sptra_node node;
};

union SPTRAllocator {
	struct {
		uint64_t size; //Nombre d'éléments alloués
		uint64_t root;
	};
	union sptra_entry entries;
};

#ifndef __is_test_unit
extern union SPTRAllocator sptr_alct;
#endif

void      sptra_init();
void      sptra_add_tree(uint64_t tad);
//Renvoie:
// 0 si non trouvé
// 1 si trouvé dans un arbre
// 2 si trouvé dans la chaine
uint8_t   sptra_find_ref(uint64_t tfd, uint64_t** rt);
uint64_t* sptra_leftmost (uint64_t* root);
uint64_t* sptra_rightmost(uint64_t* root);
void      sptra_rem_tree(uint64_t trm);
bool      sptra_check_tree();
size_t    sptra_size_tree(uint64_t root);

struct sptr_hd* sptr_at(uint64_t idx);
uint64_t sptr_alloc();
void     sptr_free(uint64_t idx);
size_t   sptr_nb_alloc();

#endif
