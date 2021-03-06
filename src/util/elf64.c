#include <util/elf64.h>

static inline char hexa_digit(uint8_t p){
    return (p<10 ? '0': ('A'-10)) + p;
}

static void int64_to_str_hexa(char* out, uint64_t v) {
    for(size_t i=16; i;){
        --i;
        out[i] = hexa_digit(v&0xf);
        v = v >> 4;
    }
}

static void int32_to_str_hexa(char* out, uint32_t v) {
    for(size_t i=8; i;){
        --i;
        out[i] = hexa_digit(v&0xf);
        v = v >> 4;
    }
}

void elf_readinfo(string_writer wt, void* wt_i, void* elf_begin){
    Elf64_Ehdr* ehdr = (Elf64_Ehdr*) elf_begin;
    unsigned char* shdrs = NULL;
    char num[9] = "________";
    char num64[17] = "________________";
    char* shtr = NULL; //string section

    //e_entry: virtual adress
    //e_phoff: ofs program header
    //e_shoff: ofs section header
    //e_ehsize: elf header size
    //e_phentsize: size prog header entries
    //e_phnum: nb prog header entries
    //e_shentsize: size sect header entries
    //e_shnum: nb sect header entries

    if(ehdr->e_shnum)
        shdrs = ((unsigned char*)elf_begin) + ehdr->e_shoff;

    (*wt)(wt_i, "phnum=");
    int32_to_str_hexa(num, ehdr->e_phnum);
    (*wt)(wt_i, num);

    (*wt)(wt_i, " shnum=");
    int32_to_str_hexa(num, ehdr->e_shnum);
    (*wt)(wt_i, num);

    (*wt)(wt_i, " shentsize=");
    int32_to_str_hexa(num, ehdr->e_shentsize);
    (*wt)(wt_i, num);

    (*wt)(wt_i, " shtrndx=");
    int32_to_str_hexa(num, ehdr->e_shtrndx);
    (*wt)(wt_i, num);

    (*wt)(wt_i, " entry=");
    int64_to_str_hexa(num64, ehdr->e_entry);
    (*wt)(wt_i, num64);

    (*wt)(wt_i, "\n");

    if(ehdr->e_shtrndx != SHN_UNDEF){
        Elf64_Shdr* shdr = (Elf64_Shdr*)
            (shdrs + ehdr->e_shtrndx * ehdr->e_shentsize);
        shtr = ((char*)elf_begin) + shdr->sh_offset;
    }

    for(size_t i=0; i<ehdr->e_shnum; ++i){
        Elf64_Shdr* shdr = (Elf64_Shdr*)(shdrs + i * ehdr->e_shentsize);
        (*wt)(wt_i, "S ");
        if(shtr)
            (*wt)(wt_i,shtr + shdr->sh_name);

        (*wt)(wt_i,": flags=");
        int64_to_str_hexa(num64, shdr->sh_flags);
        (*wt)(wt_i, num64);

        (*wt)(wt_i,": type=");
        int32_to_str_hexa(num, shdr->sh_type);
        (*wt)(wt_i, num);

        (*wt)(wt_i, " : from=");
        int64_to_str_hexa(num64, shdr->sh_offset);
        (*wt)(wt_i, num64);

        (*wt)(wt_i, " : sz=");
        int64_to_str_hexa(num64, shdr->sh_size);
        (*wt)(wt_i, num64);

        (*wt)(wt_i, " : to=");
        int64_to_str_hexa(num64, shdr->sh_addr);
        (*wt)(wt_i, num64);

        (*wt)(wt_i, "\n");
    }
}

Elf64_Addr elf_load(struct elf_loader el, void* el_i, void* elf_begin){
    Elf64_Ehdr* ehdr = (Elf64_Ehdr*) elf_begin;
    unsigned char* shdrs = NULL;

    if(ehdr->e_shnum)
        shdrs = ((unsigned char*)elf_begin) + ehdr->e_shoff;

    for(size_t i=0; i<ehdr->e_shnum; ++i){
        Elf64_Shdr* shdr = (Elf64_Shdr*)(shdrs + i * ehdr->e_shentsize);
        if (shdr->sh_flags & SHF_ALLOC){
            if(shdr->sh_type == SHT_NOBITS) //.bss
                (*el.fill0)(el_i, shdr->sh_flags,
						shdr->sh_addr, shdr->sh_size);
            else
                (*el.copy)(el_i, shdr->sh_flags, shdr->sh_addr,
                        ((unsigned char*)elf_begin) + shdr->sh_offset,
                        shdr->sh_size);
        }
    }
    return ehdr->e_entry;
}
