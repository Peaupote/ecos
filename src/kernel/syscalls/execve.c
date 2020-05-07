#include <kernel/sys.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>

#include <kernel/memory/kmem.h>
#include <kernel/memory/shared_pages.h>
#include <kernel/kutil.h>
#include <kernel/gdt.h>
#include <kernel/int.h>

#include <util/elf64.h>
#include <util/misc.h>

#include <libc/string.h>
#include <libc/unistd.h>
#include <libc/fcntl.h>
#include <libc/errno.h>

#define AUX_STACK_SIZE 0x1000

#define LOAD_SPACE_MIN   0x200000
#define LOAD_SPACE_MAX   (paging_add_lvl(pgg_pd, \
							USER_STACK_PD - USER_STACK_PDSZ))

// Utilisé pour marquer les pages qui restent écrivables pour l'userspace
#define PAGING_FLAG_RW PAGING_FLAG_Y3

// Zone de transfert
// permet de conserver des données lors du changement de paging
struct section {
    uint_ptr   dst;
    size_t     sz;
    bool       copy;
    bool       write;
    off_t      src;
};
struct execve_tr {
    uint8_t    stack[AUX_STACK_SIZE];
    uint8_t    phase;
    Elf64_Ehdr ehdr;
    size_t     nb_sections;
    uint_ptr   args_bg;
    uint_ptr   args_ed;
    uint_ptr   args_t_bg;
    int        argc;
    char**     argv;
    char**     envv;
    sigset_t   sigblk_save;
	int        errno;
	struct section sections[];
};

static inline struct execve_tr* trf() {
    return (struct execve_tr*) paging_acc_pd(PML4_PSKD, 0, 0);
}
static inline struct section* sections() {
	return trf()->sections;
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

uint8_t execve_do_salloc(uint_ptr bg, uint_ptr ed,
		uint16_t flags, uint16_t p_flags) {
	for (uint_ptr it = bg & PAGE_MASK; it < ed; it += PAGE_SIZE) {
		uint64_t* query = kmem_acc_pts_entry(it, pgg_pt,
							flags | PAGING_FLAG_W);
		if (!query) return ~0;
		if (!((*query) & PAGING_FLAG_P))
			*query = SPAGING_ALLOC0 | p_flags;
	}
	return 0;
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
    proc_t* mp = cur_proc();
    pid_t ppid = mp->p_ppid;
    proc_t *pp = state.st_proc + ppid;
    --pp->p_nchd;
    free_pid(state.st_curr_pid);

    proc_set_curr_pid(ppid);
    pp->p_stat = RUN;
    pp->p_reg.r.rax.i = -1;
	set_errno(trf()->errno);
    free_tr();
    iret_to_proc(pp);
}
void proc_execve_error_2() {
    proc_t* mp = cur_proc();
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
    trf()->phase = 1;
}

// --Ring 1 -> 0 defined in int7Ecall.S
extern void call_proc_execve_error_1(void);
extern void call_proc_execve_error_2(void);
extern void call_execve_switch_pml4(void);
extern void call_proc_execve_end(void);
extern uint8_t call_kmem_paging_alloc_rng(uint_ptr bg, uint_ptr ed,
                    uint16_t flags, uint16_t p_flags);
extern void call_execve_tr_do_alloc_pg(uint_ptr page_v_addr);
extern uint8_t call_execve_do_salloc(uint_ptr bg, uint_ptr ed,
					uint16_t flags, uint16_t p_flags);

// --Ring 1--

static inline bool execve_tr_alloc_pg_s(uint_ptr v_addr) {
    v_addr &= PAGE_MASK;
    if (v_addr >= ((uint_ptr)trf()) + (PAGE_ENT/2) * PAGE_SIZE)
		return false;
    if ((*paging_page_entry(v_addr)) & PAGING_FLAG_P) return true;
    call_execve_tr_do_alloc_pg(v_addr);
    return true;
}

static inline bool execve_tr_alloc_pg(uint_ptr v_addr) {
    v_addr &= PAGE_MASK;
    if (v_addr >= tr_lim()) return false;
    if ((*paging_page_entry(v_addr)) & PAGING_FLAG_P) return true;
    call_execve_tr_do_alloc_pg(v_addr);
    return true;
}


static bool read_bytes(int fd, void* buf, size_t count) {
    while (count > 0) {
        int rt = read(fd, buf, count);
        if (rt == 0 || !~rt) return false;
        count -= rt;
        buf   += rt;
    }
    return true;
}
static int read_line(int fd, char* bg, uint_ptr* lim) {
	int rct = 0;
	while (true) {
		if ((uint_ptr)bg + 256 > *lim) {
			if (!execve_tr_alloc_pg(*lim))
				return -1;
			*lim += PAGE_SIZE;
		}
		int rc = read(fd, bg, 256);
		if (!~rc) return -1;
	
		if (!rc) return rct;
		for (size_t i = 0; i < 256; ++i)
			if (bg[i] == '\n')
				return rct + i;
		
		bg  += rc;
		rct += rc;
	}
}
static inline bool execve_lseek(int fd, off_t ofs) {
    return ~lseek(fd, ofs, SEEK_SET);
}

// Chargement des sections
bool execve_alloc_rng(bool write, uint_ptr bg, size_t sz) {
    uint16_t   f  = PAGING_FLAG_U | PAGING_FLAG_W,
               fp = write ? f | PAGING_FLAG_RW : f;
    return bg + sz >= bg
		&& bg >= LOAD_SPACE_MIN
        && bg + sz <= LOAD_SPACE_MAX
        && !call_kmem_paging_alloc_rng(bg, bg + sz, f, fp);
}
bool execve_salloc_rng(bool write, uint_ptr bg, size_t sz) {
    uint16_t   f  = PAGING_FLAG_U | PAGING_FLAG_W,
               fp = PAGING_FLAG_U | (write ? PAGING_FLAG_W : 0);
    return bg + sz >= bg
		&& bg >= LOAD_SPACE_MIN
        && bg + sz <= LOAD_SPACE_MAX
		&& !call_execve_do_salloc(bg, bg + sz, f, fp);
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
		uint_ptr lim = align_to((uint_ptr)sections(), PAGE_SIZE);
        if (shdr.sh_flags & SHF_ALLOC){
            size_t n_s  = trf()->nb_sections;
			uint_ptr mp = (uint_ptr)(sections() + (n_s + 1));
			if (mp > lim) {
				if (!execve_tr_alloc_pg_s(lim))
					return false;
				lim += PAGE_SIZE;
			}
            struct section* s = sections() + n_s;
            s->dst   = shdr.sh_addr;
            s->sz    = shdr.sh_size;
            s->write = shdr.sh_flags & SHF_WRITE;
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
        if (!(s->copy
			?	execve_alloc_rng(s->write, s->dst, s->sz)
			 &&	execve_copy(fd, s->dst, s->src, s->sz)
			:	execve_salloc_rng(s->write, s->dst, s->sz) ))
				return false;
    }
    for (size_t i_s = 0; i_s < trf()->nb_sections; ++i_s) {
        struct section* s = sections() + i_s;
        if (!s->write) { // read only sections
            for (uint_ptr adr = s->dst & PAGE_MASK;
                    adr < s->dst + s->sz; adr += PAGE_SIZE) {
                uint64_t* e = paging_page_entry(adr);
                if ( !(PAGING_FLAG_RW & *e) )
					clear_flag_64(e, PAGING_FLAG_W);
            }
        }
		if (!s->copy) { // allocated bss sections
            for (uint_ptr adr = s->dst & PAGE_MASK;
                    adr < s->dst + s->sz; adr += PAGE_SIZE) {
                uint64_t* e = paging_page_entry(adr);
                if (PAGING_FLAG_P & *e) {
					uint_ptr ed = min_uint_ptr(s->dst + s->sz, 
									           adr + PAGE_SIZE);
					for (uint_ptr it = max_uint_ptr(s->dst, adr);
							it < ed; ++it)
						*(char*)it = 0;
				}
            }
		}
    }
#ifndef PROC_RO_ARGS
	// Readd flag W on args
    for (uint_ptr dst = trf()->args_bg & PAGE_MASK;
            dst < trf()->args_ed; dst += PAGE_SIZE)
		*paging_page_entry(dst) |= PAGING_FLAG_W;
#endif
	// Remove flag Y3
    for (size_t i_s = 0; i_s < trf()->nb_sections; ++i_s) {
        struct section* s = sections() + i_s;
        if (s->write)
            for (uint_ptr adr = s->dst & PAGE_MASK;
                    adr < s->dst + s->sz; adr += PAGE_SIZE)
				clear_flag_64(paging_page_entry(adr), PAGING_FLAG_RW);
	}
    return true;
}


// Transfert des arguments

static int unescp_tks(char* sr, char** wtr) { //*wtr <= *str
	char* wt  = *wtr;
	int count = 0;

	while (true) {
		while (*sr == ' ') ++sr;
		bool nempty = *sr;
		while (*sr && *sr != ' ') {
			if (*sr == '\\') {
				if (!*++sr) break;
				else if (*sr == ' ' || *sr == '\\')
					*wt++ = *sr++;
				else {
					*wt++ = '\\';
					*wt++ = *sr++;
				}
			} else *wt++ = *sr++;
		}
		bool stop = !*sr;
		if (nempty) {
			*wt++ = '\0';
			++count;
		}
		if (stop) {
			*wtr = wt;
			return count;
		}
		++sr;
	}
}

static int execve_count_args(char** dst, const char* args[]) {
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

static bool execve_copy_args(char** p_dst, int argc,
        const char* sarg[], char* darg[], uint_ptr pt_ofs) {
    char* dst = *p_dst;
    uint_ptr limp = align_to((uint_ptr)dst, PAGE_SIZE);

    for (int i = 0; i < argc; ++i) {
        const char* arg_it = sarg[i];
        darg[i]       = (char*) (((uint_ptr)dst) + pt_ofs);

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

static inline int execve_shift_sargs(
		uint_ptr shift, uint_ptr aofs,
		uint_ptr bg, uint_ptr* ed, 
		int argc0, char* sargs[], int sargsc[]) {
	int argc = 0;
	for (int i = 0; i < argc0; ++i)
		argc += sargsc[i];

	uint_ptr plim = align_to(*ed, PAGE_SIZE);
	while (*ed + shift + argc * sizeof(char*) > plim) {
		if (!execve_tr_alloc_pg(plim)) return -1;
		plim += PAGE_SIZE;
	}

	for (uint_ptr it = *ed; it > bg;) {
		--it;
		*(char*)(it + shift) = *(char*)it;
	}
	*ed += shift;

	char** sag = (char**)*ed;
	for (int i = 0; i < argc0; ++i) {
		char* ait = sargs[i];
		for (int j = 0; j < sargsc[i]; ++j) {
			klogf(Log_verb, "execve", "sarg %d:%d=%s",
					i,j,ait);
			*sag++ = (char*)(shift + aofs + (uint_ptr)ait);
			while (*ait++);
		}
	}
	return argc;
}

static inline bool execve_tr_args(const char* args[], const char* envs[],
		uint_ptr sargs_ed, int sargc0, char* sargs[], int sargsc[]) {
	// emplacement des arguments pour le processus
    trf()->args_bg = 0;
    for (size_t s = 0; s < trf()->nb_sections; ++s)
        maxa_uint_ptr(&trf()->args_bg,
                sections()[s].dst + sections()[s].sz);
#ifdef PROC_RO_ARGS
    trf()->args_bg = align_to(trf()->args_bg, PAGE_SIZE);
#endif
    trf()->args_bg = align_to(trf()->args_bg, alignof(char**));

	// emplacement dans la zone de transfert
	uint_ptr shift = trf()->args_bg & PAGE_OFS_MASK;
	uint_ptr sbg0  = trf()->args_t_bg;
    trf()->args_t_bg += shift;
	if (trf()->args_t_bg >= sargs_ed 
			&& !execve_tr_alloc_pg(trf()->args_t_bg))
		return false;

	sargs_ed = align_to(sargs_ed, alignof(char**));

    uint_ptr aofs     = trf()->args_bg - trf()->args_t_bg;
	int sargc = execve_shift_sargs(shift, aofs, sbg0, &sargs_ed,
						sargc0, sargs, sargsc);
	if (sargc < 0)
		return false;

    char**  atab = ((char**)sargs_ed) + sargc;

    char** targv = atab;
    trf()->argv  = (char**)(sargs_ed + aofs);
	int argc0    = execve_count_args(atab, args);
    if (argc0 == -1) return false;
    trf()->argc  = argc0 + sargc;
	// on garde le NULL à la fin des arguments
    atab[argc0]  = NULL;
    atab += argc0 + 1;

    char** tenvv = atab;
    trf()->envv = (char**)(((uint_ptr)tenvv) + aofs);
    int envc = execve_count_args(atab, envs);
    if (envc == -1) return false;
    atab[envc] = NULL;
    atab += envc + 1;

    char* vtab = (char*)atab;
    if (!execve_copy_args(&vtab, argc0, args, targv, aofs)
     || !execve_copy_args(&vtab, envc,  envs, tenvv, aofs))
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
    proc_t  *p = cur_proc();

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
         ini_ind < PAGE_ENT; ++ini_ind) {
        *paging_acc_pdpt(PML4_PSKD, ini_ind) = 0;
    }
    trf()->args_t_bg = ((uint_ptr)trf()) + (PAGE_ENT/2) * PAGE_SIZE;
	execve_tr_do_alloc_pg(trf()->args_t_bg);

    trf()->phase = 0;

    // Initialisation du processus auxiliaire
    ep->p_ppid = pid;
    ep->p_nchd = 0;
    ep->p_stat = RUN;
    ep->p_ring = 1;
    ep->p_prio = p->p_prio;
    for (size_t i = 0; i < NFD; i++) ep->p_fds[i] = -1;
    ep->p_pml4 = p->p_pml4;
    ep->p_reg.rsp.p = trf()->stack + AUX_STACK_SIZE;
    ep->p_reg.rip.p = &proc_execve_entry;
    ep->p_reg.r.rdi = fname;
    ep->p_reg.r.rsi = argv;
    ep->p_reg.r.rdx = env;
	trf()->errno    = SUCC;
    ep->p_errno     = &trf()->errno;
	ep->p_werrno    = 0;
    strncpy(ep->p_cmd, (char*)fname.p, 256);
    ep->p_cino = p->p_cino;
    ep->p_dev  = p->p_dev;

    // On arrête processus appelant en le passant en BLOCK
	proc_self_block(p);
    ++p->p_nchd;
    trf()->sigblk_save   = p->p_shnd.blk;
    p->p_shnd.blk        = 0;
	// permet de retrouver le processus auxiliaire associé
    p->p_reg.r.rdi.pid_t = epid;

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

	char*  sargs[EXECVE_FOLLOW_MAX];
	int    sargsc[EXECVE_FOLLOW_MAX];
	size_t sit = EXECVE_FOLLOW_MAX;
	char header[2];
	uint_ptr tralim = trf()->args_t_bg;
	char* nsarg = (char*)trf()->args_t_bg;

	// #!
	while (true) {
		if (!read_bytes(fd, header, 2)) {
			close(fd);
			call_proc_execve_error_1();
		}
		if (header[0] == '#' && header[1] == '!') {
			if (!sit) goto err_1;
			sargs[--sit] = nsarg;

			int rc = read_line(fd, nsarg, &tralim);
			if (rc < 0) goto err_1;
			nsarg[rc]     = '\0';

			close(fd);

			sargsc[sit] = unescp_tks(sargs[sit], &nsarg);
			if (!sargsc[sit]) {
				trf()->errno = EINVAL;
				goto err_1;
			}

			klogf(Log_info, "execve", "follow %s (%d args)",
					sargs[sit], sargsc[sit] - 1);
			fd = open(sargs[sit], READ);
			if (fd == -1) call_proc_execve_error_1();
		} else break;
	}

    // Lecture des headers du fichier ELF
	memcpy(&trf()->ehdr, header, 2);
    trf()->nb_sections = 0;
    if (!read_bytes(fd, ((uint8_t*)&trf()->ehdr)+2, sizeof(Elf64_Ehdr)-2))
		goto err_1;

	unsigned char ident[4] = {0x7f, 'E', 'L', 'F'};
	if (memcmp(ident, trf()->ehdr.e_ident, 4)) {
		trf()->errno = ENOEXEC;
		goto err_1;
	}

    if (!execve_read_sections(fd))
		goto err_1;

    // Transfert des arguments
    if (!execve_tr_args(args, envs, (uint_ptr)nsarg,
				EXECVE_FOLLOW_MAX - sit,
				sargs + sit, sargsc + sit))
		goto err_1;

    call_execve_switch_pml4();

    if (!execve_load_args() || !execve_load_sections(fd)) {
        close(fd);
        call_proc_execve_error_2();
    }

    close(fd);
    call_proc_execve_end();

err_1:
	cur_proc()->p_errno = NULL;
	close(fd);
	call_proc_execve_error_1();
}

// Ring 0
void proc_execve_end() {
    proc_t* mp      = cur_proc();
    proc_t* pp      = state.st_proc + mp->p_ppid;
    pp->p_reg.rip.p = (void*)trf()->ehdr.e_entry;

    pp->p_reg.rsp.p   = make_proc_stack();
    pp->p_reg.r.rdi.i = trf()->argc;
    pp->p_reg.r.rsi.p = trf()->argv;
    pp->p_reg.r.rdx.p = trf()->envv;
    pp->p_pml4        = mp->p_pml4;
#ifdef PROC_RO_ARGS
    pp->p_brkm = pp->p_brk = align_to(trf()->args_ed, PAGE_SIZE);
#else
    pp->p_brkm = pp->p_brk = trf()->args_ed;
#endif
    pp->p_shnd.usr    = NULL;
    pp->p_shnd.blk    = trf()->sigblk_save;
    //on hérite p_shnd.ign
    pp->p_shnd.dfl    = ~pp->p_shnd.ign;
    strncpy(pp->p_cmd, mp->p_cmd, 256);
	pp->p_errno       = NULL;
	pp->p_werrno      = 0;

    --pp->p_nchd;
    free_pid(state.st_curr_pid);
    pp->p_stat = RUN;

    // Libération de l'ancien paging
    kmem_free_paging_range(paging_acc_pdpt(PML4_COPY_RES, 0),
            pgg_pml4, PML4_END_USPACE);
    kmem_free_page(PAGE_MASK & *paging_acc_pml4(PML4_COPY_RES));

	// Ajout de la libc
	uint64_t* lc_e = kmem_acc_pts_entry(paging_set_lvl(pgg_pd, PD_LIBC),
								pgg_pd, PAGING_FLAG_W | PAGING_FLAG_U);
	struct sptr_hd* hd = sptr_at(libc_shared_idx);
	*lc_e	= SPAGING_VALUE(libc_shared_idx)
			| PAGING_FLAG_W  | PAGING_FLAG_U;
	++hd->count;

    free_tr();
    proc_set_curr_pid(mp->p_ppid);
    klogf(Log_info, "syscall", "end execve - return to %d", mp->p_ppid);
    iret_to_proc(pp);
}

void proc_execve_abort(pid_t aux_pid) {
    switch_proc(aux_pid);
    uint8_t phase = trf()->phase;
    for (int fd = 0; fd < NFD; fd++) sys_close(fd);
    proc_t* mp = state.st_proc + aux_pid;
    free_tr();
    if (phase == 1)
        kmem_free_paging(mp->p_pml4, kernel_pml4);
	free_pid(aux_pid);
}
