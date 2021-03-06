#ifndef _H_SYS
#define _H_SYS

/*
 * Appels systèmes
 */

#include <headers/sys.h>

#ifndef ASM_FILE

#include <headers/types.h>
#include <headers/signal.h>

#include <libc/string.h>

#include "proc.h"
#include "memory/shared_pages.h"


// --vérification des arguments pointeurs --
static inline bool check_arg_ubound(uint_ptr bg, uint64_t sz) {
    return bg + sz >= bg
        && bg + sz <= paging_set_lvl(pgg_pml4, PML4_END_USPACE);
}

/**
 * Vérifie que la plage est dans la plage de l'userspace
 * Un accès PEUT provoquer un #PF non rattrapé
 */
static inline bool check_argb(void* pbg, uint64_t sz) {
    uint_ptr bg = (uint_ptr)pbg;
    return cur_proc()->p_ring < 3 || check_arg_ubound(bg, sz);
}

/**
 * Vérifie que le processus peut accéder à la plage en lecture
 * Si vrai, un accès en lecture ne peut pas provoquer de #PF
 */
static inline bool check_argR(void* pbg, uint64_t sz) {
    uint_ptr bg = (uint_ptr)pbg;
    return cur_proc()->p_ring < 3
        || (check_arg_ubound(bg, sz)
            && (PAGING_FLAG_P & get_range_flags(bg & PAGE_MASK, bg + sz)));
}

/**
 * Vérifie que le processus peut accéder à la plage en lecture et écriture
 * Si vrai, un accès ne peut pas provoquer de #PF
 */
static inline bool check_argW(void* pbg, size_t sz) {
    uint_ptr bg = (uint_ptr)pbg;
    return cur_proc()->p_ring < 3
        || (check_arg_ubound(bg, sz)
            && ((PAGING_FLAG_P | PAGING_FLAG_W) &
                    get_range_flags(bg & PAGE_MASK, bg + sz))
                == (PAGING_FLAG_P | PAGING_FLAG_W) );
}

/**
 * Vérifie que la chaine de caractère ne dépasse pas maxlen et est dans
 * l'userspace
 * Provoque un #PF si une page n'est pas présente
 * renvoie < 0 si l'adresse n'est pas valide, la longueur de la chaine sinon
 */
static inline ssize_t check_argstrR(const char* pbg, size_t maxlen) {
	if (cur_proc()->p_ring < 3) return strlen(pbg);
	size_t len = 0;
	while (len <= maxlen && check_arg_ubound((uint_ptr)pbg, sizeof(char))) {
		if (!*pbg) return len;
		++pbg;
		++len;
	}
	return -1;
}



int      sys_usleep(usecond_t tm);
void     lookup_end_sleep(void);

pid_t    sys_wait(int* status);
pid_t    sys_waitpid(pid_t cpid, int* status);
pid_t    sys_fork(void);

__attribute__ ((noreturn))
void     sys_exit(int status);

pid_t    sys_getpid(void);
pid_t    sys_getppid(void);

int      sys_open(const char* fname, int oflags, int perms);
int      sys_close(int fd);
int      sys_dup(int fd);
int      sys_dup2(int fd1, int fd2);
int      sys_pipe(int fds[2]);
ssize_t  sys_write(int fd, uint8_t *s, size_t len);
ssize_t  sys_read(int fd, uint8_t *buf, size_t len);
off_t    sys_lseek(int fd, off_t offset, int whence);
int      sys_mkdir(const char *fname, mode_t mode);
int      sys_link(const char *path1, const char *path2);
int      sys_symlink(const char *path1, const char *path2);
int      sys_readlink(const char *path, char *buf, size_t len);

int      sys_execve(reg_t fname, reg_t argv, reg_t env);

int      sys_fstat(int fd, struct stat *st);
int      sys_chdir(const char *fname);

int      sys_setpriority(int prio);
int      sys_getpriority();

void*    sys_sbrk(intptr_t inc);

void     sys_sigsethnd(sighandler_t);

__attribute__ ((noreturn))
void     sys_sigreturn();

void     sys_seterrno(int *errno);

// 0 <= sigid < SIG_COUNT
// hnd[0]: ign, hnd[1]: dfl
uint8_t  sys_signal(int sigid, uint8_t hnd);
int      sys_kill(pid_t pid, int signum);

int      sys_pause();

uint64_t invalid_syscall();

void     prires_proc_struct(uint16_t new_ring);

#endif

#endif
