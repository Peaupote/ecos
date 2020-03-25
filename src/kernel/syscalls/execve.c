#include <kernel/sys.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>
#include <kernel/memory/kmem.h>
#include <kernel/memory/shared_pages.h>
#include <util/elf64.h>
#include <kernel/kutil.h>
#include <kernel/gdt.h>
#include <util/misc.h>

//TODO: vrai système de fichier
extern uint8_t edummy_args[];
off_t edummy_pos;
void edummy_open() {
	edummy_pos = 0;
}
void edummy_read() {
    proc_t *p  = state.st_proc + state.st_curr_pid;
    uint8_t *d = (uint8_t*)p->p_reg.rsi;
    size_t len = p->p_reg.rdx;
	for (off_t i = 0; i < len; ++i)
		d[i] = edummy_args[edummy_pos + i];
	edummy_pos  += len;
	p->p_reg.rax = len;
}
void edummy_close() {}
void edummy_lseek() {
	edummy_pos = state.st_proc[state.st_curr_pid].p_reg.rsi;
}

// Zone de transfert
// permet de conserver des données lors du changement de paging
struct execve_tr {
	Elf64_Ehdr ehdr;
	size_t     nb_sections;
	uint_ptr   args_bg;
	uint_ptr   args_ed;
	uint_ptr   args_t_bg;
	int        argc;
	char**     argv;
	char**     envv;
};
struct section {
	uint_ptr   dst;
	size_t     sz;
	bool       copy;
	off_t      src;
};

static inline struct execve_tr* trf() {
	return (struct execve_tr*) paging_acc_pd(PML4_PSKD, 0, 0);
}
static inline struct section* sections() {
	return (struct section*) align_to(
			((uint_ptr)trf()) + sizeof(struct execve_tr),
			alignof(struct section));
}
static inline uint_ptr tr_lim() {
	return (uint_ptr) paging_acc_pd(PML4_PSKD + 1, 0, 0);
}

static inline void execve_tr_do_alloc_pg(uint_ptr page_v_addr) {
	*paging_page_entry(page_v_addr) = kmem_alloc_page()
					| PAGING_FLAG_W | PAGING_FLAG_P;
	invalide_page(page_v_addr);
}
static inline bool execve_tr_alloc_pg(uint_ptr v_addr) {
	v_addr &= PAGE_MASK;
	if (v_addr >= tr_lim()) return false;
	if ((*paging_page_entry(v_addr)) & PAGING_FLAG_P) return true;
	execve_tr_do_alloc_pg(v_addr);
	return true;
}
bool read_bytes(proc_t* p, int fd, void* buf, size_t count) {
	while (count > 0) {
		p->p_reg.rdi = fd;
		p->p_reg.rsi = (uint64_t)buf;
		p->p_reg.rdx = count;
		edummy_read();
		if (p->p_reg.rax == 0 || !~p->p_reg.rax) return false;
		count -= p->p_reg.rax;
		buf   += p->p_reg.rax;
	}
	return true;
}
bool execve_lseek(proc_t* p, int fd, off_t ofs) {
	p->p_reg.rdi = fd;
	p->p_reg.rsi = ofs;
	edummy_lseek();
	return ~p->p_reg.rax;
}

static inline bool execve_alloc_rng(uint_ptr bg, size_t sz) {
	if (bg + sz < bg || paging_get_lvl(pgg_pml4, bg + sz) >= PML4_END_USPACE)
		return false;
	return !kmem_paging_alloc_rng(bg, bg + sz,
				PAGING_FLAG_U | PAGING_FLAG_W,
				PAGING_FLAG_U | PAGING_FLAG_W);
}

static inline void execve_fill0(uint_ptr bg, size_t sz) {
	for (size_t i = 0; i < sz; ++i)
		*((uint8_t*)(bg + i)) = 0;
}

static inline bool execve_copy(proc_t *p, int fd,
		uint_ptr dst, off_t src, size_t sz) {
	if (!execve_lseek(p, fd, src)) return false;
	while (sz > 0) {
		p->p_reg.rdi = fd;
		p->p_reg.rsi = dst;
		p->p_reg.rdx = sz;
		edummy_read();
		if (p->p_reg.rax == 0 || !~p->p_reg.rax) return false;
		sz  -= p->p_reg.rax;
		dst += p->p_reg.rax;
	}
	return true;
}

static inline bool execve_read_sections(proc_t* p, int fd) {
	for (size_t i_s = 0; i_s < trf()->ehdr.e_shnum; ++i_s) {
		Elf64_Shdr shdr;
		if (!execve_lseek(p, fd, 
					trf()->ehdr.e_shoff + i_s * trf()->ehdr.e_shentsize)
			|| !read_bytes(p, fd, &shdr, sizeof(Elf64_Shdr))) {
			return false;
		}
        if (shdr.sh_flags & (SHF_WRITE | SHF_ALLOC | SHF_EXECINSTR)){
			size_t n_s = trf()->nb_sections;
			if (!execve_tr_alloc_pg( (uint_ptr)(sections() + (n_s + 1)) - 1))
				return false;
			struct section* s = sections() + n_s;
			s->dst = shdr.sh_addr;
			s->sz  = shdr.sh_size;
            if (shdr.sh_type == SHT_NOBITS) //.bss
				s->copy = false;
			else {
				s->copy = true;
				s->src  = shdr.sh_offset;
			}
			++trf()->nb_sections;
        }
	}
	return true;
}

static inline bool execve_load_sections(proc_t* p, int fd) {
	for (size_t i_s = 0; i_s < trf()->nb_sections; ++i_s) {
		struct section* s = sections() + i_s;
		if (!execve_alloc_rng(s->dst, s->sz)) return false;
		if (s->copy) {
            if (!execve_copy(p, fd, s->dst, s->src, s->sz))
				return false;
		} else
			execve_fill0(s->dst, s->sz);
	}
	return true;
}

int execve_count_args(char** dst, const char* args[]) {
	const char** arg_it = args;
	uint_ptr limp = align_to((uint_ptr)dst, PAGE_SIZE);
	goto loop_entry;
	do {
		limp += PAGE_SIZE;
loop_entry:
		while ((uint_ptr)dst < limp) {
			if (! *arg_it) return arg_it - args;
			++dst;
			++arg_it;
		}
	} while(execve_tr_alloc_pg((uint_ptr)dst));
	return -1;
}

bool execve_copy_args(char** p_dst, int argc,
		const char* sarg[], char* darg[], uint_ptr pt_ofs) {
	char* dst = *p_dst;
	for (int i = 0; i < argc; ++i) {
		const char* arg_it = sarg[i];
		darg[i] = (char*) (((uint_ptr)dst) + pt_ofs);
		uint_ptr limp = align_to((uint_ptr)dst, PAGE_SIZE);
		goto loop_entry;
		do {
			limp += PAGE_SIZE;
loop_entry:
			while ((uint_ptr)dst < limp) {
				*(dst++) = *arg_it;
				if (! *arg_it) goto loop_exit;
				++arg_it;
			}
		} while(execve_tr_alloc_pg((uint_ptr)dst));
		return false;
loop_exit:;
	}
	*p_dst = dst;
	return true;
}

static inline void free_tr() {
	for (uint16_t i = 0; i < PAGE_ENT; ++i) {
		uint64_t e = *paging_acc_pdpt(PML4_PSKD, i);
		if (e & PAGING_FLAG_P)
			kmem_free_page(e & PAGE_MASK);
	}
	uint64_t *pd = paging_acc_pml4(PML4_PSKD);
	kmem_free_page((*pd) & PAGE_MASK);
	*pd = 0;
}

static inline bool execve_tr_args(const char* args[], const char* envs[]) {
	trf()->args_bg = 0;
	for (size_t s = 0; s < trf()->nb_sections; ++s)
		maxa_uint_ptr(&trf()->args_bg, sections()[s].dst + sections()[s].sz);
	trf()->args_bg = align_to(trf()->args_bg, alignof(char**));

	trf()->args_t_bg = align_to(
			(uint_ptr)(sections() + trf()->nb_sections), PAGE_SIZE);
	if (!execve_tr_alloc_pg(trf()->args_t_bg)) return false;
	trf()->args_t_bg += trf()->args_bg & PAGE_OFS_MASK;
	char**   atab = (char**) trf()->args_t_bg;
	uint_ptr aofs = trf()->args_bg - trf()->args_t_bg;

	char** targv = atab;
	trf()->argv = (char**)(((uint_ptr)targv) + aofs);
	trf()->argc = execve_count_args(atab, args);
	if (trf()->argc == -1) return false;
	atab[trf()->argc] = NULL; //TODO: inutile ?
	atab += trf()->argc + 1;
	
	char** tenvv = atab;
	trf()->envv = (char**)(((uint_ptr)tenvv) + aofs);
	int envc = execve_count_args(atab, envs);
	if (envc == -1) return false;
	atab[envc] = NULL;
	atab += envc + 1;

	char* vtab = (char*)atab;
	if (!execve_copy_args(&vtab, trf()->argc, args, targv, aofs)
	 || !execve_copy_args(&vtab, envc, envs, tenvv, aofs))
		return false;
	trf()->args_ed = ((uint_ptr)vtab) + aofs;
	
	return true;
}

static inline bool execve_load_args() {
	uint_ptr src = trf()->args_t_bg & PAGE_MASK;
	for (uint_ptr dst = trf()->args_bg & PAGE_MASK;
			dst < trf()->args_ed; dst += PAGE_SIZE, src += PAGE_SIZE) {
		uint64_t* e = paging_page_entry(src);
		if (paging_map_to(dst, PAGE_MASK & *e,
					PAGING_FLAG_W | PAGING_FLAG_U,
					PAGING_FLAG_W | PAGING_FLAG_U))
			return false;
		*e = 0;
	}
	return true;
}

static inline bool execve_load_elf64(proc_t* p, int fd,
		const char* args[], const char* envs[],
		phy_addr old_pml4, phy_addr* npml4,
		uint_ptr* rip) {

	// Zone de transfert
	volatile phy_addr tr_pd = kmem_alloc_page();
	*paging_acc_pml4(PML4_PSKD) = tr_pd 
		| PAGING_FLAG_W | PAGING_FLAG_P;
	invalide_page((uint_ptr)paging_acc_pdpt(PML4_PSKD, 0));

	for (uint16_t i = 0; i < PAGE_ENT; ++i)
		*paging_acc_pdpt(PML4_PSKD, i) = 0;
	for (uint_ptr i = 0; i < sizeof(struct execve_tr); i += PAGE_SIZE)
		execve_tr_do_alloc_pg(((uint_ptr)trf()) + i);

	// Lecture des headers du fichier ELF
	trf()->nb_sections = 0;
	if (!read_bytes(p, fd, (uint8_t*)&trf()->ehdr, sizeof(Elf64_Ehdr))
			//TODO: check
			|| !execve_read_sections(p, fd)) {
		free_tr();
		return false;
	}

	// Transfert des arguments
	if (!execve_tr_args(args, envs)) {
		free_tr();
		return false;
	}

	//TODO Clear int
	*npml4 = kmem_alloc_page();
	kmem_bind_dynamic_slot(0, *npml4);
	kmem_init_pml4(kmem_dynamic_slot(0), *npml4);
	uint64_t* npml4_v      = (uint64_t*)kmem_dynamic_slot(0);
	npml4_v[PML4_PSKD]     = tr_pd
	       | PAGING_FLAG_P | PAGING_FLAG_W;
	npml4_v[PML4_COPY_RES] = old_pml4
	       | PAGING_FLAG_P | PAGING_FLAG_W;
	pml4_to_cr3(*npml4);

	if (!execve_load_args() || !execve_load_sections(p, fd)) {
		free_tr();
		kmem_free_paging(*npml4, old_pml4);
		*paging_acc_pml4(PML4_PSKD) = 0;
		return false;
	}

	*rip = trf()->ehdr.e_entry;

	return true;
}

int execve(const char *fname, const char **argv, const char **env) {
	klogf(Log_info, "syscall", "EXECVE");
	pid_t pid = state.st_curr_pid;
    proc_t *p = state.st_proc + pid;
	
	pid_t npid = find_new_pid(pid);
	if (npid == pid)
		return -1;

	// TODO : real process, cli
	proc_t *np = state.st_proc + npid;
	np->p_stat = RUN;
	for (int i = 0; i < NFD; ++i)
		np->p_fds[i] = UNUSED;

	state.st_curr_pid = npid;
    st_curr_reg = &np->p_reg;

	np->p_reg.rdi = p->p_reg.rdi;
	np->p_reg.rsi = READ;
	edummy_open();
	int fd = (int)p->p_reg.rax;
	if (fd == -1) {
		np->p_stat = FREE;
		return -1;
	}

	phy_addr npml4;

	if (!execve_load_elf64(np, fd, argv, env, p->p_pml4, &npml4, 
				&p->p_reg.rip)) {
		np->p_reg.rdi = fd;
		edummy_close();
		np->p_stat = FREE;
		return -1;
	}
	
	np->p_reg.rdi = fd;
	edummy_close();

	uint64_t* stack_pd = kmem_acc_pts_entry(
			paging_add_lvl(pgg_pd, USER_STACK_PD),
			2, PAGING_FLAG_U | PAGING_FLAG_W);
	*stack_pd = SPAGING_FLAG_P | PAGING_FLAG_W;
    p->p_reg.rsp = paging_add_lvl(pgg_pd, USER_STACK_PD + 1);

	p->p_reg.rdi = trf()->argc;
	p->p_reg.rsi = (uint_ptr)trf()->argv;
	p->p_reg.rdx = (uint_ptr)trf()->envv;

	p->p_pml4    = npml4;
	state.st_curr_pid = pid;
    st_curr_reg  = &p->p_reg;
	np->p_stat = FREE;

	// Libération de l'ancien paging
	kmem_free_paging_range(paging_acc_pdpt(PML4_COPY_RES, 0),
			4, PML4_END_USPACE);
	kmem_free_page(PAGE_MASK & *paging_acc_pml4(PML4_COPY_RES));

	free_tr();
	iret_to_proc(p);
	never_reached return 0;
}
