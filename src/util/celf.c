#include <util/celf.h>

void celf_load(struct elf_loader el, void* el_i, void* elf_begin) {
	char* it = elf_begin;
	Celf_Ehdr* ehdr = (Celf_Ehdr*) it;
	it += sizeof(Celf_Ehdr);

	for (size_t i = 0; i < ehdr->nD; ++i) {
		Celf_SDhdr* shdr = (Celf_SDhdr*) it;
		el.copy(el_i, SHF_ALLOC, shdr->dst,
				  shdr->src + (char*)elf_begin, shdr->sz);
		it += sizeof(Celf_SDhdr);
	}
	
	for (size_t i = 0; i < ehdr->n0; ++i) {
		Celf_S0hdr* shdr = (Celf_S0hdr*) it;
		el.fill0(el_i, SHF_WRITE, shdr->dst, shdr->sz);
		it += sizeof(Celf_S0hdr);
	}
}
