#ifndef _H_DEV
#define _H_DEV

#include <headers/file.h>
#include <headers/devices.h>

#include <stdint.h>
#include <kernel/param.h>

#define MIADT_SIZE 18

/**
 * Informations about the mount point
 */
struct mount_info {
    void    *sp; // pointer to superblock
    uint32_t block_size;
    uint32_t root_ino;

    // some padding for each file system to store diverse informations
    char space[MIADT_SIZE] __attribute__((aligned(8)));
};

#define ROOT_DEV 0

/**
 * Une device est un point de montage pour un système de fichier
 */
struct device {
    dev_t             dev_id;

    dev_t             dev_parent_dev;
    ino_t             dev_parent_red; // ino du dossier remplacé par le dev
    ino_t             dev_parent_ino;

    // liste chainée des dev montés comme enfants
    dev_t             dev_childs;
    dev_t             dev_sib;

    uint8_t           dev_free;
    uint8_t           dev_fs;
    struct mount_info dev_info;
} devices[NDEV];

struct device *find_device(const char *path);

#endif
