#ifndef _H_DEV
#define _H_DEV

#include <stdint.h>
#include <kernel/param.h>

typedef uint32_t dev_t;

struct device_info {
    uint32_t block_size;
    uint32_t nb_block;
    uint32_t nb_inodes;
    uint32_t nb_free_inodes;
};

struct device {
    dev_t     dev_id;
    char      dev_mnt[256];
    uint8_t   dev_fs;
    void     *dev_spblk; // pointer to super block
} devices[NDEV];

#endif
