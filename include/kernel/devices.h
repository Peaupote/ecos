#ifndef _H_DEV
#define _H_DEV

#include <headers/devices.h>

#include <stdint.h>
#include <kernel/param.h>

//TODO #include <sys/stat.h>

struct mount_info {
    void    *sp;
    uint32_t block_size;
    uint32_t root_ino;
};

struct device {
    dev_t             dev_id;
    char              dev_mnt[256];
    uint8_t           dev_free;
    uint8_t           dev_fs;
    struct mount_info dev_info;
} devices[NDEV];

struct device *find_device(const char *path);

#endif