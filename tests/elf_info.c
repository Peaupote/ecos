#include <stdio.h>
#include <stdlib.h>

#include <util/elf64.h>

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

int main(int argc, char **argv){
	char *file_name;
	FILE* file = NULL;
	unsigned char* data = NULL;
	if(argc <= 1){
		printf("ERR: aucun fichier en argument\n");
		return 1;
	}
	file_name = argv[1];
	printf("FICHIER: %s\n", file_name);
	file = fopen(file_name, "r");
	if(!file){
		printf("ERR: impossible d'ouvrir le fichier\n");
		return 1;
	}
	printf("SIZE: %ld\n", load_file(file, &data));
	if(!data){
		printf("ERR: load\n");
		fclose(file);
		return 1;
	}
	fclose(file);
	print_infos(data);
	free(data);
	return 0;
}
