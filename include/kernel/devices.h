#ifndef _H_DEV
#define _H_DEV

#include <stdint.h>
#include <kernel/param.h>

#if defined(__is_kernel)
typedef uint32_t dev_t;
#else
#include <sys/stat.h>
#endif

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
