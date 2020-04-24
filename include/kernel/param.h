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

// how often the scheduler kicks in
// careful ! it's a modulo implemented with a bitwise &
#define SCHED_FREQ 0x3

#define TTY_CURSOR_T 24

// Nombre de priorités (nice) dans le scheduler
// Au sein du kernel la meilleure priorité est NB_PRIORITY_LVL-1, la pire 0
// On réserve la priorité NB_PRIORITY_LVL-1 au processus STOP
// NB_PRIORITY_LVL <= 64
#define NB_PRIORITY_LVL 41

// maximum number of chars accumulated in a pipe
#define PIPE_SZ   2048
#define STREAM_SZ 2048

/**
 * Dont touch too much here
 */
#define NGDT 4
