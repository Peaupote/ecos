#include <util/multiboot.h>
#include <util/elf64.h>
#include <util/misc.h>
#include <util/paging.h>

//Informations fournies par GRUB
extern multiboot_info_t* mb_info;

extern void* kernel_entry_addr;
extern uint32_t end_kernel;

extern uint8_t KPA[];
extern uint8_t KPA_end[];

extern uint32_t page_t_23[2*1024];

const char kernel_cmd[] = "KERNEL_BIN";

static inline void* virt2phys(Elf64_Addr a){
    // base du kernel dans les adresses virtuelles: KVA,
    // effacée par le cast
    // KPA: adresse physique du kernel
    return (void*)(KPA + ((uint32_t) a));
}

uint8_t* virt2phys_section(Elf64_Xword flags, Elf64_Addr a, uint64_t sz){
    uint8_t* p_dst = (uint8_t*) virt2phys(a);
	if (p_dst + sz > KPA_end) {
		//TODO ERROR
		while(1) asm("hlt");
	}
	if (flags & SHF_WRITE) {
		for (uint32_t addr = ((uint32_t)a) & PAGE_MASK;
				addr < (uint32_t)(a + sz);
				addr += PAGE_SIZE)
			page_t_23[2*(addr >> PAGE_SHIFT)] |= PAGING_FLAG_W;
	}
	maxa_uint32_t(&end_kernel, ((uint32_t) p_dst) + sz);
    return p_dst;
}

void simp_fill0(void* none __attribute__((unused)), Elf64_Xword flags,
		Elf64_Addr dst, uint64_t sz){
    uint8_t* p_dst = virt2phys_section(flags, dst, sz);
    for(uint64_t i=0; i<sz; ++i)
        p_dst[i] = 0;

}
void simp_copy(void* none __attribute__((unused)), Elf64_Xword flags,
		Elf64_Addr dst, void* src, uint64_t sz){
    uint8_t* p_dst = virt2phys_section(flags, dst, sz);
    for(uint64_t i=0; i<sz; ++i)
        p_dst[i] = ((uint8_t*) src)[i];
}

struct VbeInfoBlock {
   char VbeSignature[4];             // == "VESA"
   uint16_t VbeVersion;                 // == 0x0300 for VBE 3.0
   uint16_t OemStringPtr[2];            // isa vbeFarPtr
   uint8_t Capabilities[4];
   uint16_t VideoModePtr[2];         // isa vbeFarPtr
   uint16_t TotalMemory;             // as # of 64KB blocks
} __attribute__((packed));

static int ustrcmp(const char *lhs, const char *rhs) {
    while(*lhs && *rhs){
        if ((unsigned char)*lhs < (unsigned char)*rhs)
            return -1;
        if ((unsigned char)*lhs > (unsigned char)*rhs)
            return 1;
        ++lhs;
        ++rhs;
    }
    if(*rhs) return -1;
    if(*lhs) return  1;
    return 0;
}

void load_kernel64(void){
    struct elf_loader el_v = {.fill0=&simp_fill0, .copy=&simp_copy};
    kernel_entry_addr = NULL;
    end_kernel = 0;

    multiboot_uint32_t flags = mb_info->flags;
    if(flags & MULTIBOOT_INFO_MODS){
        multiboot_uint32_t mods_count = mb_info->mods_count;
        multiboot_uint32_t mods_addr  = mb_info->mods_addr;

        for (uint32_t mod = 0; mod < mods_count; mod++) {
            multiboot_module_t* module = (multiboot_module_t*)
                (mods_addr + (mod * sizeof(multiboot_module_t)));
            const char* module_string = (const char*)module->cmdline;
            if(!ustrcmp(module_string, kernel_cmd)){
                void* elf_begin = (void*)module->mod_start;
                kernel_entry_addr = virt2phys(elf_load(el_v, NULL, elf_begin));
                return;
            }
        }
    }
}
