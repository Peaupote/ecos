On précise l'appel système que l'on souhaite déclencher en plançant son
identifiant dans rax avant de lancer l'interruption.
Les arguments sont passés selon les conventions d'appel C, c'est à dire dans 
les registres rdi, rsi, rdx, rcx, r8, r9 et dans cet ordre. La valeur de retour
est renvoyée dans rax. Pour éviter les complications, on s'arrangera pour ne
jamais utiliser plus de 6 arguments.

Les identifiants associés à chaque appel système sont décrit dans le fichier
[sys.h](../include/headers/sys.h)

int 0x80
--------

 - `int      usleep(usecond_t time)`
 - `pid_t    fork()`
 - `void     exit(int status)`
 - `pid_t    wait(int* status)`

    	renvoit le status du fils ssi le pointeur est non nul
 - `pid_t    waitpid(pid_t cpid, int* status)`
 
        si pid == -1:
    	    attend qu'un enfant quelconque termine ou stoppe
        sinon
    	    attend que l'enfant pid termine ou stoppe
 - `pid_t    getpid()`
 - `pid_t    getppid()`
 - `int      open(char *fname, int mode)`
 - `int      close(int fd)`
 - `int      dup(int fd)`
 - `int      dup2(int fd1, int fd2)`
 - `int      pipe(int fds[2])`
 - `int      write(int fd, void *buf, size_t len)`
 - `int      read(int fd, void *buf, size_t len)`
 - `int      lseek(int offset)`
 - `int      execve(char *fname, char **argv, char **env)`
 - `int      fstat(int fd, struct stat *st)`
 - `int      mkdir(const char *fname, mode_t mode)`
 - `int      chdir(const char *fname)`
 - `int      setpriority(int prio)`
 - `int      getpriority()`
 - `void*    sbrk(intptr_t inc)`
 - `void     sigsethnd(sighandler_t)`
 - `void     sigreturn()`
 - `uint8_t  signal(int sigid, uint8_t hnd)`
 - `int      kill(pid_t pid, int signum)`
 - `void     seterrno(int *errno)`
 - `int      pause()`
 - `int      link(const char *path1, const char *path2)`
 - `int      symlink(const char *path1, const char *path2)`
 - `int      readlink(const char *path, char *buf, size_t len)`


Appels systèmes accessibles avec le niveau de privilège ring <= 1:

int 0x7f
--------

 - `void     prires(uint16_t new_ring)`

         passe en ring new_ring
 - `void     ttyown(pid_t pid)`

         désigne le processus propriétaire du tty qui recevra les signaux
         
int 0x7e
--------

appelle la fonction dont l'adresse est passée dans `%rax`
