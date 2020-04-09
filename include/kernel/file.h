#ifndef _H_FILE
#define _H_FILE

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
#define PROC_FS  1
#define EXT2_FS  2

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
typedef uint32_t (fs_lookup_t)(const char *fname, ino_t parent,
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
 * Read/Write in file specified by given ino
 * return the number of char read or < 0 if error
 */
typedef int (fs_rdwr_t)(ino_t, void*, off_t, size_t, struct mount_info*);

/**
 * Returns a pointer to the first element in the directory
 * p2 is a location that may be used to store the entry
 * p3 is a location that may be used to store the entry's name (max 255)
 */
typedef struct dirent_it *(fs_opendir_t)(ino_t, struct dirent_it*, char*,
		struct mount_info*);

typedef struct dirent_it *(fs_readdir_t)(struct dirent_it*, char*);

/**
 * Remove a dir entry with ino in list of dir entries of parent
 */
typedef ino_t (fs_destroy_dirent_t)(ino_t parent, ino_t ino, struct mount_info*);
typedef ino_t (fs_rm_t)(ino_t, struct mount_info*);

typedef ino_t (fs_readsymlink_t)(ino_t, char*, struct mount_info*);

struct fs {
    char                 fs_name[4];
    fs_mnt_t            *fs_mnt;
    fs_lookup_t         *fs_lookup;
    fs_stat_t           *fs_stat;
    fs_rdwr_t           *fs_read;
    fs_rdwr_t           *fs_write;
    fs_create_t         *fs_touch;
    fs_create_t         *fs_mkdir;
    fs_opendir_t        *fs_opendir;
    fs_readdir_t        *fs_readdir;
    fs_rm_t             *fs_rm;
    fs_destroy_dirent_t *fs_destroy_dirent;
    fs_readsymlink_t    *fs_readsymlink;
} fst [NFST];

void vfs_init();
int  vfs_mount(const char *path, uint8_t fs, void *partition);

vfile_t *vfs_pipe();

vfile_t *vfs_load(const char *path, int flags);

int      vfs_close(vfile_t *vfile);
int      vfs_read(vfile_t *vfile, void *buf, off_t pos, size_t len);
int      vfs_write(vfile_t *vfile, void *buf, off_t pos, size_t len);

vfile_t *vfs_touch(const char *parent, const char *fname, mode_t perm);
vfile_t *vfs_mkdir(const char *parent, const char *fname, mode_t perm);

struct dirent_it *vfs_opendir(vfile_t *vf, struct dirent_it *it, char* nbf);
struct dirent_it *vfs_readdir(vfile_t *vf, struct dirent_it *it, char* nbf);

ino_t vfs_rmdir(const char *fname, uint32_t rec);

#endif
