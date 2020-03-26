#ifndef _GDT_H
#define _GDT_H

#include <util/gdt.h>

#define GDT_RING_OFS   0x08
#define GDT_RING_SZ    0x10

#define GDT_RING0_CODE 0x08
#define GDT_RING0_DATA 0x10
#define GDT_RING1_CODE 0x18
#define GDT_RING1_DATA 0x20
#define GDT_RING3_CODE 0x38
#define GDT_RING3_DATA 0x40

#ifndef ASM_FILE
#include <stdint.h>

struct GDT {//size = 0x48
	uint64_t null;
	uint64_t ring0_code; //0x08
	uint64_t ring0_data; //0x10
	uint64_t ring1_code; //0x18
	uint64_t ring1_data; //0x20
	uint64_t tss_low;
	uint64_t tss_high;
	uint64_t ring3_code; //0x38
	uint64_t ring3_data; //0x40
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
extern uint8_t         kernel_stack_top[];
extern struct GDT      gd_table;
extern struct GDT_desc gdt_desc;

extern struct TSS      tss;

void tss_init(void);
void gdt_init(void);

static inline uint16_t gdt_ring_lvl(uint8_t lvl) {
	return SEG_SEL(GDT_RING_OFS + ((uint16_t)GDT_RING_SZ) * lvl, lvl);
}

#endif
#endif
