#include "kmem.h"

phy_addr next_page;

void kmem_init() {
	// On cherche la prochaine page disponible
	next_page = (end_kernel & PAGE_OFS_MASK) ? 
					(end_kernel & PAGE_MASK) + PAGE_SIZE
				   : end_kernel;
}

phy_addr kmem_alloc_page() {
	phy_addr pa = next_page;
	next_page += PAGE_SIZE;
	return pa;
}
