#ifndef _GDT_H
#define _GDT_H

#include <stdint.h>

struct GDT {//size = 0x38
	uint64_t null;
	uint64_t kernel_code; //0x08
	uint64_t kernel_data; //0x10
	uint64_t ring3_code;  //0x18
	uint64_t ring3_data;  //0x20
	uint64_t tss_low;
	uint64_t tss_high;
} __attribute__((packed));

struct GDT_desc {//size = 0xa
	uint16_t   limit;
	struct GDT *base;
} __attribute__((packed));

struct TSS {//size = 0x68
	uint32_t : 32;
	void*    rsp[3];
	uint64_t : 64;
	void*    ist[7]; //Interrupt Stack Table: IST1 - IST7
	uint64_t : 64;
	uint16_t : 16;
	uint16_t iomap_base;
} __attribute__((packed));

//from boot_data
extern struct GDT      gd_table;
extern struct GDT_desc gdt_desc;

extern struct TSS      tss;

void tss_init(void);
void gdt_init(void);

#endif
