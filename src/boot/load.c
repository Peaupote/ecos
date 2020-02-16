#include "multiboot.h"

#include "../util/vga.h"
#include "../util/string.h"
#include "../util/elf64.h"

//Informations fournies par GRUB
//https://www.gnu.org/software/grub/manual/multiboot/html_node/Boot-information-format.html
extern multiboot_info_t* mb_info;

extern void* kernel_entry_addr;

const char kernel_cmd[] = "KERNEL_BIN";

void simp_fill0(void* none __attribute__((unused)), void* dst, uint64_t sz){
	for(uint64_t i=0; i<sz; ++i)
		((unsigned char*) dst)[i] = 0;
}
void simp_copy(void* none __attribute__((unused)), void* dst,
		void* src, uint64_t sz){
	for(uint64_t i=0; i<sz; ++i)
		((unsigned char*) dst)[i] = ((unsigned char*) src)[i];
}

void load_main(void){
	char info_str[]="\0_._._._\n";
	struct elf_loader el_v = {&simp_fill0, &simp_copy};
	kernel_entry_addr = NULL;

	terminal_initialize();
	terminal_writestring("Hello world !\n\tinfo=");
	
	multiboot_uint32_t flags = mb_info->flags;
	if(flags & MULTIBOOT_INFO_MODS){
		multiboot_uint32_t mods_count = mb_info->mods_count;
		multiboot_uint32_t mods_addr  = mb_info->mods_addr;
		int32_to_str_hexa(info_str, mods_count);
		terminal_writestring(info_str);
 
		for (uint32_t mod = 0; mod < mods_count; mod++) {
			multiboot_module_t* module = (multiboot_module_t*)
				(mods_addr + (mod * sizeof(multiboot_module_t)));
			const char* module_string = (const char*)module->cmdline;
			terminal_writestring("\tmodule:\"");
			terminal_writestring(module_string);
			terminal_writestring("\"\n");
			if(!strcmp(module_string, kernel_cmd)){
				void* elf_begin = (void*)module->mod_start;
			
				terminal_writestring("kernel found\n");
				terminal_writestring("\tmod_start:");
				ptr_to_str(info_str, (void*)module->mod_start);
				terminal_writestring(info_str);
				terminal_writestring("\tmod_end:");
				ptr_to_str(info_str, (void*)module->mod_end);
				terminal_writestring(info_str);

				kernel_entry_addr = elf_load(el_v, NULL, elf_begin);
				elf_readinfo(&terminal_writer, NULL, elf_begin);
				return;
			}
		}
	}
}
