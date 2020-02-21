#!/usr/bin/awk -f

BEGIN{ofs=0;}
{
	base  ="(" $1 ")";
	limit ="(" $2 ")";
	access="(" gensub(/[[:upper:]]/, "GDT_ACC_&", "g", $3) ")";
	flags ="(" gensub(/[[:upper:]]/, "GDT_FLAG_&", "g", $4) ")";
	printf("movl $GDT_ENTRY_0(%s, %s), (gd_table + %d)\n",
			base, limit, ofs);
	ofs += 4;
	printf("movl $GDT_ENTRY_1(%s, %s, %s, %s), (gd_table + %d)\n",
			base, limit, access, flags, ofs);
	ofs += 4;
}
