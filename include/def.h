
#ifndef _DEF_H
#define _DEF_H

//#define BDI //Boot Debug Info

#define PML4_KERNEL_VIRT_ADDR 0x80

//Doit avoir les 32 bits de poids faible nuls
#define KERNEL_VIRT_ADDR (((uint64_t)PML4_KERNEL_VIRT_ADDR) << 39);

#endif
