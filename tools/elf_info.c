#include <stdio.h>
#include <stdlib.h>

#include <util/elf64.h>
#include <util/paging.h>
#include <util/misc.h>

void wstdio(void* none __attribute__((unused)), const char* str){
    printf("%s", str);
}

long load_file(FILE* src, unsigned char** dt){
    long sz = 0;
    int c = 0;
    *dt = NULL;

    fseek(src, 0, SEEK_END);
    sz = ftell(src);
    if(sz<=0) return 0;
    *dt = (unsigned char*)malloc(sz);
    fseek(src, 0, SEEK_SET);

    for(int i=0; i<sz && c != EOF; ++i){
        c = fgetc(src);
        (*dt)[i] = (unsigned char)c;
    }

    return sz;
}

void print_infos(void* dt){
    elf_readinfo(&wstdio, NULL, dt);
}

int print_file_info(const char* file_name) {
    FILE* file = NULL;
    unsigned char* data = NULL;
    printf("FICHIER: %s\n", file_name);
    file = fopen(file_name, "r");
    if(!file){
        printf("ERR: impossible d'ouvrir le fichier\n");
        return 2;
    }
    printf("SIZE: %ld\n", load_file(file, &data));
    if(!data){
        printf("ERR: load\n");
        fclose(file);
        return 2;
    }
    fclose(file);
    print_infos(data);
    free(data);
	return 0;
}

void cmp_end_0(void* rt, Elf64_Xword f __attribute__((unused)),
		Elf64_Addr dst, uint64_t sz) {
	maxa_uint_ptr((uint_ptr*)rt, dst + sz);
}
void cmp_end_cp(void* rt, Elf64_Xword f __attribute__((unused)),
		Elf64_Addr dst, void* src __attribute__((unused)), uint64_t sz) {
	maxa_uint_ptr((uint_ptr*)rt, dst + sz);
}
struct elf_loader v_cm_end = {&cmp_end_0, &cmp_end_cp};

uint_ptr cmp_end(void* dt){
    uint_ptr rt;
	elf_load(v_cm_end, &rt, dt);
	return rt;
}

uint_ptr cmp_end_file(const char* file_name) {
    FILE* file = NULL;
    unsigned char* data = NULL;

    file = fopen(file_name, "r");
    if(!file){
        printf("ERR: impossible d'ouvrir le fichier\n");
        return 2;
    }
    load_file(file, &data);
    if(!data){
        printf("ERR: load\n");
        fclose(file);
        return 2;
    }
    fclose(file);
	printf("%p", cmp_end(data));
    free(data);
	return 0;
}

int main(int argc, char **argv){
    if(argc <= 2) printf("ERR: argument manquant");
	if(argv[1][0] == 'e')
		return cmp_end_file(argv[2]);
    return print_file_info(argv[2]);
}
