#ifndef _H_FILE
#define _H_FILE

#include <stdbool.h>

#include <headers/proc.h>
#include <headers/file.h>

#include <kernel/param.h>
#include <kernel/devices.h>

typedef struct vfile {
    struct stat vf_stat;    // information about the file
    uint8_t     vf_cnt;     // nb of channel pointing at the file
    cid_t       vf_waiting; // first channel waiting to read something
} vfile_t;

// File system table
#define NFST 3
#define DUMMY_FS 0
#define PROC_FS 1
#define EXT2_FS  2

#define CADT_SIZE 16

typedef struct {
    char d[CADT_SIZE] __attribute__((aligned(8)));
} chann_adt_t;

/**
 * First parameter is a pointer to the beginning of the partition
 * mount fills the mount_info structure
 * return 0 if fails 1 otherwise
 */
typedef int (fs_mnt_t)(void*, struct mount_info*);

/**
 * Look for file named fname in directory with specified inode
 * return the file's inode if exists 0 otherwise
 */
typedef uint32_t (fs_lookup_t)(ino_t parent, const char *fname,
                               struct mount_info *info);

/**
 * Fill the given stat structure with corresponding information
 * return < 0 if fail
 */
typedef int (fs_stat_t)(ino_t ino, struct stat*, struct mount_info *info);

/**
 * Create a new empty file or a directory (containing '.' and '..')
 * with given name as child of parent
 * fail (returns 0) if parent is not a directory
 * returns inode of new file
 */
typedef uint32_t (fs_create_t)(ino_t parent, const char *fname, uint16_t type,
                               struct mount_info *info);

/**
 * Remove content of the file, update the inode consequently
 * but leave user and permissions unchanged
 * return inode number
 */
typedef uint32_t (fs_truncate_t)(ino_t ino, struct mount_info *info);

/**
 * Read/Write in file specified by given ino
 * return the number of char read, -1 if error, -2 if wait
 */
typedef int (fs_rdwr_t)(ino_t, void*, off_t, size_t, struct mount_info*);


/**
 * Lit un nombre entier de dirents
 * Si il n'y a pas assez de place pour stocker le premier dirent,
 * stocker les champs excepté d_name et d_name_len,
 * le prochain appel relit la même entrée.
 * Dans tous les cas si rc > 0, rc >= offsetof(struct dirent, d_name)
 *
 * p3: nombre maximal d'octet à lire
 */
typedef int (fs_getdents_t)(ino_t, struct dirent*, size_t,
                            chann_adt_t*,  struct mount_info*);

/**
 * Indique la création d'un nouveau canal vers le fichier
 */
typedef void (fs_opench_t)(ino_t, chann_adt_t*,  struct mount_info*);

typedef void (fs_open_t)(ino_t, vfile_t*, struct mount_info*);
typedef void (fs_close_t)(ino_t, struct mount_info*);

/**
 * Remove a dir entry with ino in list of dir entries of parent
 */
typedef ino_t (fs_destroy_dirent_t)(ino_t parent, ino_t ino, struct mount_info*);
typedef ino_t (fs_rm_t)(ino_t, struct mount_info*);

typedef ino_t (fs_readsymlink_t)(ino_t, char*, struct mount_info*);

struct fs {
    char                 fs_name[8];
    fs_mnt_t            *fs_mnt;
    fs_lookup_t         *fs_lookup;
    fs_stat_t           *fs_stat;
    fs_rdwr_t           *fs_read;
    fs_rdwr_t           *fs_write;
    fs_create_t         *fs_touch;
    fs_create_t         *fs_mkdir;
    fs_truncate_t       *fs_truncate;
    fs_getdents_t       *fs_getdents;
    fs_opench_t         *fs_opench;
    fs_open_t           *fs_open;
    fs_close_t          *fs_close;
    fs_rm_t             *fs_rm;
    fs_destroy_dirent_t *fs_destroy_dirent;
    fs_readsymlink_t    *fs_readsymlink;
} fst [NFST];

void vfs_init();
int  vfs_mount_root(uint8_t fs, void* partition);
int  vfs_mount(const char *path, uint8_t fs, void *partition);

// rt[0]: read end ie output
// rt[1]: write end ie input
uint32_t vfs_pipe(vfile_t* rt[2]);

// *pathend doit être '/' ou '\0'
bool vfs_find(const char* path, const char* pathend, dev_t* dev, ino_t* ino);
vfile_t *vfs_load(const char *path, int flags);
void     vfs_opench(vfile_t *vf, chann_adt_t* cdt);

void vfs_unblock(vfile_t* vfile);

int   vfs_close(vfile_t *vfile);
int   vfs_read(vfile_t *vfile, void *buf, off_t pos, size_t len);
int   vfs_write(vfile_t *vfile, void *buf, off_t pos, size_t len);
ino_t vfs_create(const char *fname, mode_t perm);
ino_t vfs_truncate(vfile_t *vfile);
int   vfs_getdents(vfile_t *vf, struct dirent* dst, size_t sz,
                   chann_adt_t* cdt);

int   vfs_rm(const char *fname);
ino_t vfs_rmdir(const char *fname, uint32_t rec);

#endif
