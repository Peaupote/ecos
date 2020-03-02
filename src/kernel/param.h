/**
 * Some global definitions
 * values can be updated if ever needed
 */

#define NPROC 100  // number of processes
#define NFD   128  // number of file descriptors
#define NCHAN 1000 // number of channels
#define INODE 100  // number of inodes
#define NHEAP 10   // number of enqued processes

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
 * Dont touch to much here
 */
#define NGDT 4
