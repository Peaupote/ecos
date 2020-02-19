#include "kmem.h"

phy_addr next_page;

void kmem_init() {
	// On cherche la prochaine page disponible
	next_page = (phy_addr) end_kernel;
	if (next_page & PAGE_OFS_MASK)
		next_page = (next_page & PAGE_MASK) + PAGE_SIZE;
}

phy_addr kmem_alloc_page() {
	phy_addr pa = next_page;
	next_page += PAGE_SIZE;
	return pa;
}

uint8_t paging_map_to(uint_ptr v_addr, phy_addr p_addr){
	uint64_t query_addr = 
		(uint64_t) paging_acc_pml4(paging_get_pml4(v_addr));
	
	for(uint8_t i=0; i<3; i++) {
		uint64_t* query = (uint64_t*) query_addr;
		if(! ((*query) & 0b1) ){
			phy_addr new_struct = kmem_alloc_page();
			*query = new_struct | 0x3;

			query_addr = (query_addr << 9) & VADDR_MASK;
			for(uint16_t i=0; i<512; ++i)
				((uint64_t*) query_addr)[i] = 0x2;
		} else
			query_addr = (query_addr << 9) & VADDR_MASK;
		query_addr |= (v_addr >> ((3-i) * 9)) & 0xff8;
	}

	uint64_t* query = (uint64_t*) query_addr;
	if( (*query) & 0b1) return 1; //Déjà assignée
	*query = p_addr | 0x3;
	return 0;
}
