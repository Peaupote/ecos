	_ofs     = 0;

	/* Stack */
_ofs = ALIGN(_ofs, 0x10);
kba_stk_bt   = _ofs;
	_ofs    += 0x4000;		/* 16 KB */
kba_stk_tp   = _ofs;

	/* Paging */
_ofs = ALIGN(_ofs, 0x1000);
kba_pml4     = _ofs;
	_ofs    += 0x1000;		/* 4KB: 512 entrees de 64bits */
kba_pdpt_1   = _ofs;
	_ofs    += 0x1000;
kba_pd_1     = _ofs;
	_ofs    += 0x1000;
kba_pt_23    = _ofs;
	_ofs    += 0x2000;		/* 2 tables: 8KB */

	/* GDT */
kba_gdt      = _ofs;
	_ofs    += (4*8);		/* 4 * 8 octets */
kba_gdtd     = _ofs;
	_ofs    += 2;

/* Contient l'adresse physique de la fin du kernel (première adresse libre) */
kba_end_krn  = _ofs;
	_ofs    += 4;

    kba_end  = _ofs;
