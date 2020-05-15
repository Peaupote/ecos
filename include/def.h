
#ifndef _DEF_H
#define _DEF_H

#define PML4_KERNEL_VIRT_ADDR 0x80

//Doit avoir les 32 bits de poids faible nuls
#define KERNEL_VIRT_ADDR (((uint64_t)PML4_KERNEL_VIRT_ADDR) << 39);

// Demande à GRUB d'être démaré en en mode graphique
#define GRAPHIC_MODE_LIN

// La partie boot essaie d'afficher les erreur sur le VGA
//#define BOOT_DISPLAY_ERR

#ifdef BOOT_DISPLAY_ERR
#undef GRAPHIC_MODE_LIN
#endif

#endif
