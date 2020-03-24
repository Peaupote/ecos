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
_ofs = ALIGN(_ofs, 0x10);
kba_gdt      = _ofs;
	_ofs    += 0x48;
_ofs = ALIGN(_ofs, 0x4);
_ofs += 2;  /*align = 2 mod 4*/
kba_gdtd     = _ofs;
	_ofs    += 0xa;

/* Contient l'adresse physique de la fin du kernel (première adresse libre) */
_ofs = ALIGN(_ofs, 4);
kba_bgn_krn = _ofs;
	_ofs    += 4;
kba_end_krn  = _ofs;
	_ofs    += 4;

    kba_end  = _ofs;
