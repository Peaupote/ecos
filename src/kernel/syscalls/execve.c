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
#include <kernel/int.h>

#include <libc/sys.h>

#define AUX_STACK_SIZE 0x1000

// Zone de transfert
// permet de conserver des données lors du changement de paging
struct execve_tr {
	uint8_t    stack[AUX_STACK_SIZE];
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


// --Ring 0--
void execve_tr_do_alloc_pg(uint_ptr page_v_addr) {
	*paging_page_entry(page_v_addr) = kmem_alloc_page()
					| PAGING_FLAG_W | PAGING_FLAG_P;
	invalide_page(page_v_addr);
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

void proc_execve_error_1() {
	proc_t* mp = state.st_proc + state.st_curr_pid;
	pid_t ppid = mp->p_ppid;
    proc_t *pp = state.st_proc + ppid;
	--pp->p_nchd;
	free_pid(state.st_curr_pid);

	proc_set_curr_pid(ppid);
	pp->p_stat = RUN;
	rei_cast(int, pp->p_reg.rax) = -1;
	free_tr();	
	iret_to_proc(pp);
}
void proc_execve_error_2() {
	proc_t* mp = state.st_proc + state.st_curr_pid;
	pid_t ppid = mp->p_ppid;
	proc_t* pp = state.st_proc + ppid;
	kmem_free_paging(mp->p_pml4, pp->p_pml4);
	proc_execve_error_1();
}

void execve_switch_pml4() {
	phy_addr npml4 = kmem_alloc_page();
	kmem_bind_dynamic_slot(0, npml4);
	kmem_init_pml4(kmem_dynamic_slot(0), npml4);
	uint64_t* npml4_v      = (uint64_t*)kmem_dynamic_slot(0);
	npml4_v[PML4_PSKD]     = *paging_acc_pml4(PML4_PSKD);
	npml4_v[PML4_COPY_RES] = state.st_proc[state.st_curr_pid].p_pml4
	       | PAGING_FLAG_P | PAGING_FLAG_W;
	pml4_to_cr3(npml4);
	state.st_proc[state.st_curr_pid].p_pml4 = npml4;
}

// --Ring 1 -> 0 defined in int7Ecall.S
extern void call_proc_execve_error_1(void);
extern void call_proc_execve_error_2(void);
extern void call_execve_switch_pml4(void);
extern void call_proc_execve_end(void);
extern uint8_t call_kmem_paging_alloc_rng(uint_ptr bg, uint_ptr ed,
					uint16_t flags, uint16_t p_flags);
extern void call_execve_tr_do_alloc_pg(uint_ptr page_v_addr);

// --Ring 1--

static inline bool execve_tr_alloc_pg(uint_ptr v_addr) {
	v_addr &= PAGE_MASK;
	if (v_addr >= tr_lim()) return false;
	if ((*paging_page_entry(v_addr)) & PAGING_FLAG_P) return true;
	call_execve_tr_do_alloc_pg(v_addr);
	return true;
}


bool read_bytes(int fd, void* buf, size_t count) {
	while (count > 0) {
		int rt = read(fd, buf, count);
		if (rt == 0 || !~rt) return false;
		count -= rt;
		buf   += rt;
	}
	return true;
}
inline bool execve_lseek(int fd, off_t ofs) {
	return ~lseek(fd, ofs);
}

// Chargement des sections
bool execve_alloc_rng(uint_ptr bg, size_t sz) {
	return bg + sz >= bg 
		&& paging_get_lvl(pgg_pml4, bg + sz) < PML4_END_USPACE
		&& !call_kmem_paging_alloc_rng(bg, bg + sz,
				PAGING_FLAG_U | PAGING_FLAG_W,
				PAGING_FLAG_U | PAGING_FLAG_W);
}

static inline void execve_fill0(uint_ptr bg, size_t sz) {
	for (size_t i = 0; i < sz; ++i)
		*((uint8_t*)(bg + i)) = 0;
}

static inline bool execve_copy(int fd, uint_ptr dst, off_t src, size_t sz) {
	return execve_lseek(fd, src) && read_bytes(fd, (void*)dst, sz);
}

static inline bool execve_read_sections(int fd) {
	for (size_t i_s = 0; i_s < trf()->ehdr.e_shnum; ++i_s) {
		Elf64_Shdr shdr;
		if (!execve_lseek(fd, 
					trf()->ehdr.e_shoff + i_s * trf()->ehdr.e_shentsize)
			|| !read_bytes(fd, &shdr, sizeof(Elf64_Shdr))) {
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

static inline bool execve_load_sections(int fd) {
	for (size_t i_s = 0; i_s < trf()->nb_sections; ++i_s) {
		struct section* s = sections() + i_s;
		if (!execve_alloc_rng(s->dst, s->sz)) return false;
		if (s->copy) {
            if (!execve_copy(fd, s->dst, s->src, s->sz))
				return false;
		} else
			execve_fill0(s->dst, s->sz);
	}
	return true;
}


// Transfert des arguments

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
		darg[i]       = (char*) (((uint_ptr)dst) + pt_ofs);
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

static inline bool execve_tr_args(const char* args[], const char* envs[]) {
	trf()->args_bg = 0;
	for (size_t s = 0; s < trf()->nb_sections; ++s)
		maxa_uint_ptr(&trf()->args_bg,
				sections()[s].dst + sections()[s].sz);
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

void proc_execve_entry(const char *fname, const char **argv, const char **env);

// --Ring 0--
int sys_execve(reg_t fname, reg_t argv, reg_t env) {
	pid_t  pid = state.st_curr_pid;
	klogf(Log_info, "syscall", "process %d called execve", (int)pid);
    proc_t  *p = state.st_proc + pid;

	pid_t epid = find_new_pid();
	if (!~epid) return -1;
	klogf(Log_verb, "syscall", "execve aux process: %d", (int)epid);

	proc_t* ep = state.st_proc + epid;

	// Allocation de la zone de transfert
	volatile phy_addr tr_pd = kmem_alloc_page();
	*paging_acc_pml4(PML4_PSKD) = tr_pd 
		| PAGING_FLAG_W | PAGING_FLAG_P;
	invalide_page((uint_ptr)paging_acc_pdpt(PML4_PSKD, 0));

	uint_ptr ini_addr = 0;
	for (; ini_addr < sizeof(struct execve_tr); ini_addr += PAGE_SIZE)
		execve_tr_do_alloc_pg(((uint_ptr)trf()) + ini_addr);
	for (uint16_t ini_ind = ini_addr >> PAGE_SHIFT;
			ini_ind < PAGE_ENT; ++ini_ind)
		*paging_acc_pdpt(PML4_PSKD, ini_ind) = 0;

	// Initialisation du processus auxiliaire
	ep->p_ppid = pid;
	ep->p_nchd = 0;
	ep->p_stat = RUN;
	ep->p_ring = 1;
	ep->p_prio = p->p_prio;
    for (size_t i = 0; i < NFD; i++) ep->p_fds[i] = -1;
	ep->p_pml4 = p->p_pml4;
	ep->p_reg.rsp = (uint_ptr)trf()->stack + AUX_STACK_SIZE;
	ep->p_reg.rip = (uint_ptr)&proc_execve_entry;
	ep->p_reg.rdi = fname;
	ep->p_reg.rsi = argv;
	ep->p_reg.rdx = env;

	// On arrête processus appelant en le passant en waitpid
	p->p_stat    = WAIT;
	p->p_reg.rax = epid;

	// On bascule sur le processus auxiliaire
	state.st_curr_pid = epid;
	st_curr_reg = &ep->p_reg;
	iret_to_proc(ep);
	never_reached return 0;
}

// Ring 1
void proc_execve_entry(const char *fname,
		const char **args, const char **envs) {

	int fd = open(fname, READ);
	if (fd == -1) call_proc_execve_error_1();

	// Lecture des headers du fichier ELF
	trf()->nb_sections = 0;
	if (!read_bytes(fd, (uint8_t*)&trf()->ehdr, sizeof(Elf64_Ehdr))
			//TODO: check
			|| !execve_read_sections(fd)) {
		close(fd);
		call_proc_execve_error_1();
	}

	// Transfert des arguments
	if (!execve_tr_args(args, envs)) {
		close(fd);
		call_proc_execve_error_1();
	}

	call_execve_switch_pml4();

	if (!execve_load_args() || !execve_load_sections(fd)) {
		close(fd);
		call_proc_execve_error_2();
	}
	
	close(fd);
	call_proc_execve_end();
}

// Ring 0
void proc_execve_end() {
	proc_t* mp = state.st_proc + state.st_curr_pid;
	proc_t* pp = state.st_proc + mp->p_ppid;
	pp->p_reg.rip = trf()->ehdr.e_entry;

	pp->p_reg.rsp = make_proc_stack();
	pp->p_reg.rdi = trf()->argc;
	pp->p_reg.rsi = (uint_ptr)trf()->argv;
	pp->p_reg.rdx = (uint_ptr)trf()->envv;
	pp->p_pml4    = mp->p_pml4;

	--pp->p_nchd;
	free_pid(state.st_curr_pid);
	pp->p_stat = RUN;
	
	// Libération de l'ancien paging
	kmem_free_paging_range(paging_acc_pdpt(PML4_COPY_RES, 0),
			4, PML4_END_USPACE);
	kmem_free_page(PAGE_MASK & *paging_acc_pml4(PML4_COPY_RES));

	free_tr();
	proc_set_curr_pid(mp->p_ppid);
	iret_to_proc(pp);
}
