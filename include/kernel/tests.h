#ifndef _KTESTS_H
#define _KTESTS_H

#include <stdint.h>

void    test_kmem(void);

void    test_idt(void);

uint8_t test_struct_layout(void);

void    test_print_statut(void);
void    test_kheap(void);

#endif
