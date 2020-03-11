#include "../util/multiboot.h"
#include "../util/elf64.h"
#include "../util/string.h"

//Informations fournies par GRUB
extern multiboot_info_t* mb_info;

extern void* kernel_entry_addr;
extern uint32_t end_kernel;

extern uint8_t KPA;

const char kernel_cmd[] = "KERNEL_BIN";

static inline void* virt2phys(Elf64_Addr a){
    // base du kernel dans les adresses virtuelles: KVA,
    // effacée par le cast
    // KPA: adresse physique du kernel
    return (void*)(&KPA + ((uint32_t) a));
}

unsigned char* virt2phys_section(Elf64_Addr a, uint64_t sz){
    unsigned char* p_dst = (unsigned char*) virt2phys(a);
	uint32_t end_s = ((uint32_t) p_dst) + sz;
	if (end_s > end_kernel) end_kernel = end_s;
	return p_dst;
}

void simp_fill0(void* none __attribute__((unused)), Elf64_Addr dst,
        uint64_t sz){
    unsigned char* p_dst = virt2phys_section(dst, sz);
    for(uint64_t i=0; i<sz; ++i)
        p_dst[i] = 0;
	
}
void simp_copy(void* none __attribute__((unused)), Elf64_Addr dst,
        void* src, uint64_t sz){
    unsigned char* p_dst = virt2phys_section(dst, sz);
    for(uint64_t i=0; i<sz; ++i)
        p_dst[i] = ((unsigned char*) src)[i];
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
