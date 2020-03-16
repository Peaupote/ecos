/**
 * Some global definitions
 * values can be updated if ever needed
 */

#define NPROC 1024 // number of processes
#define NFD   128  // number of file descriptors
#define NCHAN 1024 // number of channels
#define INODE 1024 // number of inodes
#define NHEAP 16   // number of enqued processes
#define NFILE 1024 // number of virtual files
#define NDEV  32   // number of devices mounted

#define NSLEEP 128 // number of parralled sleep allowed

// how often the scheduler kicks in
// careful ! it's a modulo implemented with a bitwise &
#define SCHED_FREQ 0x3

/**
 * priorities (the less the better)
 */
#define PFREE  100
#define PSLEEP 30
#define PWAIT  10
#define PRUN   0
#define PIDLE  -1
#define PZOMB  -10
#define PSTOP  1


/**
 * Dont touch too much here
 */
#define NGDT 4
