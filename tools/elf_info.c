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
    uint_ptr rt = 0;
	elf_load(v_cm_end, &rt, dt);
	return rt;
}

unsigned char* load_data(const char* file_name) {
    FILE* file = NULL;
    unsigned char* data = NULL;

    file = fopen(file_name, "r");
    if(!file){
        printf("ERR: impossible d'ouvrir le fichier\n");
        return NULL;
    }
    load_file(file, &data);
    if(!data){
        printf("ERR: load\n");
        fclose(file);
        return NULL;
    }
    fclose(file);
	return data;
}

int cmp_end_file(const char* file_name) {
	unsigned char* data = load_data(file_name);
	if (!data) return 1;
	printf("%p\n", cmp_end(data));
    free(data);
	return 0;
}


typedef struct {
	uint_ptr minD, maxD, min0, max0;
	uint32_t nD, n0;
	bool wt_d;
} cmpd_bnds_dt;
void cmp_bnds_0(void* rt, Elf64_Xword f __attribute__((unused)),
		Elf64_Addr dst, uint64_t sz) {
	cmpd_bnds_dt* d = rt;
	++d->n0;
	mina_uint_ptr(&d->min0, dst);
	maxa_uint_ptr(&d->max0, dst + sz);
}
void cmp_bnds_cp(void* rt, Elf64_Xword f,
		Elf64_Addr dst, void* src __attribute__((unused)), uint64_t sz) {
	cmpd_bnds_dt* d = rt;
	++d->nD;
	if (f & SHF_WRITE) d->wt_d = true;
	mina_uint_ptr(&d->minD, dst);
	maxa_uint_ptr(&d->maxD, dst + sz);
}
struct elf_loader v_cm_bnds = {&cmp_bnds_0, &cmp_bnds_cp};

int cmp_types_bounds(const char* file_name) {
	unsigned char* data = load_data(file_name);
	if (!data) return 1;
    cmpd_bnds_dt rt = {
		.minD = ~(uint_ptr)0, .maxD = 0,
		.min0 = ~(uint_ptr)0, .max0 = 0,
		.nD = 0, .n0 = 0,
		.wt_d = false
	};
	elf_load(v_cm_bnds, &rt, data);
    free(data);
	printf("%d\t%p\t%p\t%p\t%p\n", rt.wt_d ? 1 : 0,
			rt.minD, rt.maxD, rt.min0, rt.max0);
	return 0;
}

typedef struct {
	long cofs;
	FILE* f;
	bool err;
	bool is_data;
} compr_dt;
void cpr_0(void* rt, Elf64_Xword f __attribute__((unused)),
				Elf64_Addr dst, uint64_t sz) {
	compr_dt* dt = rt;
	if (dt->is_data) return;
	dt->err =  dt->err
			|| !fwrite(&dst, 8, 1, dt->f)
			|| !fwrite(&sz, 8, 1, dt->f);
}
void cpr_cp(void* rt, Elf64_Xword f __attribute__((unused)),
		Elf64_Addr dst, void* src, uint64_t sz) {
	compr_dt* dt = rt;
	if (!dt->is_data) return;
	uint64_t cofs64 = dt->cofs;
	dt->err =  dt->err
			|| !fwrite(&dst,    8, 1, dt->f)
			|| !fwrite(&sz,     8, 1, dt->f)
			|| !fwrite(&cofs64, 8, 1, dt->f);
	long iofs = ftell(dt->f);
	fseek(dt->f, dt->cofs, SEEK_SET);
	dt->cofs += sz;
	while (sz && !dt->err) {
		size_t wc = fwrite(src, 1, sz, dt->f);
		if (wc) {
			sz -= wc;
			src = wc + (char*)src;
		} else
			dt->err = true;
	}
	fseek(dt->f, iofs, SEEK_SET);
}
struct elf_loader v_cpr = {&cpr_0, &cpr_cp};

int compress(const char* input, const char* output) {
	unsigned char* data = load_data(input);
	if (!data) return 1;
    cmpd_bnds_dt rt = {
		.minD = ~(uint_ptr)0, .maxD = 0,
		.min0 = ~(uint_ptr)0, .max0 = 0,
		.nD = 0, .n0 = 0, .wt_d = false
	};
	elf_load(v_cm_bnds, &rt, data);
	if (rt.wt_d) {
		fprintf(stderr, "ERR: W data section\n");
		goto err;
	} else if (rt.minD == 0 || rt.maxD > 0x100000
			|| rt.min0 < 0x100000 || rt.max0 > 0x200000) {
		fprintf(stderr, "ERR: sections bounds\n");
		goto err;
	}
	printf("data:\t%u\t%p\t%p\nbss:\t%u\t%p\t%p\n",
			rt.nD, rt.minD, rt.maxD, rt.n0, rt.min0, rt.max0);
	FILE* out = fopen(output, "w");
	if (!out) {
		fprintf(stderr, "ERR: impossible d'ouvrir %s\n", output);
		goto err;
	}
	if(!fwrite(&rt.nD, 4, 1, out) || !fwrite(&rt.n0, 4, 1, out)) goto err;
	
	compr_dt cprdt = {
		.cofs = 2 * 4 + rt.nD * (3 * 8) + rt.n0 * (2 * 8),
		.f    = out, .err = false, .is_data = true
	};
	elf_load(v_cpr, &cprdt, data);
	cprdt.is_data = false;
	elf_load(v_cpr, &cprdt, data);
	fclose(out);
	if (cprdt.err) {
		fprintf(stderr, "ERR: writing\n");
		goto err;
	}

    free(data);
	return 0;
err:
	free(data);
	return 1;
}

int main(int argc, char **argv){
    if(argc <= 2) {
		printf("ERR: argument manquant");
		return 2;
	}
	if(argv[1][0] == 'e')
		return cmp_end_file(argv[2]);
	else if(argv[1][0] == 'b')
		return cmp_types_bounds(argv[2]);
	else if(argv[1][0] == 'c' && argc >= 4)
		return compress(argv[2], argv[3]);
    return print_file_info(argv[2]);
}
