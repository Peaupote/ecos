	kba_ofs  = 0;

	/* Paging */
kba_pml4     = kba_ofs;
	kba_ofs += 0x1000;		/* 4KB: 512 entrees de 64bits */
kba_pdpt_1   = kba_ofs;
	kba_ofs += 0x1000;
kba_pd_1     = kba_ofs;
	kba_ofs += 0x1000;
kba_pt_23    = kba_ofs;
	kba_ofs += 0x2000;		/* 2 tables: 8KB */

	/* GDT */
kba_gdt      = kba_ofs;
	kba_ofs += (4*8);		/* 4 * 8 octets */
kba_gdtd     = kba_ofs;
	kba_ofs += 2;

	/* Stack */
kba_ofs = ALIGN(kba_ofs, 0x10);
kba_stk_bt   = kba_ofs;
	kba_ofs += 0x4000;		/* 16 KB */
kba_stk_tp   = kba_ofs;

    kba_end  = kba_ofs;
