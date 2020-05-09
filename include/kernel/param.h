/**
 * Some global definitions
 * values can be updated if ever needed
 */

#define NPROC   1024 // number of processes
#define NFD     128  // number of file descriptors
#define NCHAN   1024 // number of channels
#define NHEAP   16   // number of enqued processes
#define NFILE   1024 // number of virtual files
#define NDEV    32   // number of devices mounted
#define NPIPE   32   // number of pipes
#define NSTREAM 32   // number of streams

#define NSLEEP 128 // number of parralled sleep allowed

#define PIT_FREQ         100
#define SCHED_TIME_SLICE 2

#define TTY_CURSOR_T     100
#define TTY_BUFALL_T     4

// Nombre de priorités (nice) dans le scheduler
// Au sein du kernel la meilleure priorité est NB_PRIORITY_LVL-1, la pire 0
// On réserve la priorité NB_PRIORITY_LVL-1 au processus STOP
// NB_PRIORITY_LVL <= 64
#define NB_PRIORITY_LVL 41

// Quantité maximale de mémoire qu'un processus peut allouer avec sbrk
#define PROC_MAX_BRK 0x400000 //4MB

// Nombre maximal de #! qu'execve peut suivre
#define EXECVE_FOLLOW_MAX 10

// Empêche les processus de modifier leurs arguments / env
// #define PROC_RO_ARGS
