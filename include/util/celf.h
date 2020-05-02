#ifndef _CELF_H
#define _CELF_H

#include "elf64.h"

typedef struct {
	uint32_t nD;
	uint32_t n0;
} __attribute__((packed)) Celf_Ehdr;

typedef struct {
	uint64_t dst;
	uint64_t sz;
	uint64_t src;
} __attribute__((packed)) Celf_SDhdr;

typedef struct {
	uint64_t dst;
	uint64_t sz;
} __attribute__((packed)) Celf_S0hdr;

void celf_load(struct elf_loader, void* el_i, void* elf_begin);

#endif
