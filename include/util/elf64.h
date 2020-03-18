#ifndef _ELF64_H
#define _ELF64_H

//http://www.sco.com/developers/gabi/2000-07-17/contents.html

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "string.h"

#define EI_NIDENT 16
#define SHN_UNDEF 0

typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef int32_t  Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef int64_t  Elf64_Sxword;

//Elf Header
typedef struct {
    unsigned char e_ident[EI_NIDENT];
    Elf64_Half    e_type;
    Elf64_Half    e_machine;
    Elf64_Word    e_version;
    Elf64_Addr    e_entry;
    Elf64_Off     e_phoff;
    Elf64_Off     e_shoff;
    Elf64_Word    e_flags;
    Elf64_Half    e_ehsize;
    Elf64_Half    e_phentsize;
    Elf64_Half    e_phnum;
    Elf64_Half    e_shentsize;
    Elf64_Half    e_shnum;
    Elf64_Half    e_shtrndx;
} __attribute__((packed)) Elf64_Ehdr;

//Section Header
typedef struct {
	Elf64_Word	sh_name;
	Elf64_Word	sh_type;
	Elf64_Xword	sh_flags;
	Elf64_Addr	sh_addr;
	Elf64_Off	sh_offset;
	Elf64_Xword	sh_size;
	Elf64_Word	sh_link;
	Elf64_Word	sh_info;
	Elf64_Xword	sh_addralign;
	Elf64_Xword	sh_entsize;
} __attribute__((packed)) Elf64_Shdr;

//Section Types (sh_type)
#define SHT_NULL            0
#define SHT_PROGBITS        1
#define SHT_SYMTAB          2
#define SHT_STRTAB          3
#define SHT_RELA            4
#define SHT_HASH            5
#define SHT_DYNAMIC         6
#define SHT_NOTE            7
#define SHT_NOBITS          8
#define SHT_REL             9
#define SHT_SHLIB          10
#define SHT_DYNSYM         11
#define SHT_INIT_ARRAY     14
#define SHT_FINI_ARRAY     15
#define SHT_PREINIT_ARRAY  16
#define SHT_GROUP          17
#define SHT_SYMTAB_SHNDX   18
#define SHT_LOOS   0x60000000
#define SHT_HIOS   0x6fffffff
#define SHT_LOPROC 0x70000000
#define SHT_HIPROC 0x7fffffff
#define SHT_LOUSER 0x80000000
#define SHT_HIUSER 0xffffffff

//Section Attribute Flags (sh_flags)
#define SHF_WRITE              0x1
#define SHF_ALLOC              0x2
#define SHF_EXECINSTR          0x4
#define SHF_MERGE             0x10
#define SHF_STRINGS           0x20
#define SHF_INFO_LINK         0x40
#define SHF_LINK_ORDER        0x80
#define SHF_OS_NONCONFORMING 0x100
#define SHF_GROUP            0x200
#define SHF_MASKOS      0x0ff00000
#define SHF_MASKPROC    0xf0000000

void elf_readinfo(string_writer, void*, void* elf_begin);

struct elf_loader{
	void (*fill0)(void*, Elf64_Addr, uint64_t);
	void (* copy)(void*, Elf64_Addr, void*, uint64_t);//dst,src,sz
};
Elf64_Addr elf_load(struct elf_loader, void* el_i, void* elf_begin);

#endif
