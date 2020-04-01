#include "ext2_wrap.h"

#include <util/test.h>
#include <fs/ext2.h>

static struct ext2_mount_info info;

static uint32_t curr_ino;
static struct ext2_inode *curr;

int init_ext2(void* fs) {
    int rc = ext2_mount(fs, &info);
    if (rc < 0)
		return 1;
    curr_ino = EXT2_ROOT_INO;
    curr = ext2_get_inode(EXT2_ROOT_INO, &info); // root
	return 0;
}

bool is_curr_dir() {
    return curr->in_type & EXT2_TYPE_DIR;
}

void print_type(uint16_t type) {
    printf("type : ");
    switch (type&0xf000) {
    case EXT2_TYPE_DIR: printf("directory\n"); break;
    case EXT2_TYPE_REG: printf("regular file\n"); break;
    default: printf("other\n"); break;
    }
}

void print_inode(struct ext2_inode *inode) {
    print_type(inode->in_type);
    printf("mode : %o\n", inode->in_type&0777);
    printf("uid  : %d\n", inode->in_uid);
    printf("size : %d\n", inode->in_size);
    uint32_t block_count = inode->in_size / info.block_size;
    printf("blocks:\n");
    for (size_t i = 0; i < block_count; i++) {
        printf("%ld: %d\n", i, inode->in_block[i]);
    }
    printf("total %d\n", block_count);
}

int print_dir(struct ext2_dir_entry *dir) {
    if (!dir) {
        printf("dir null\n");
        return 0;
    }
    printf("%d (%d) %*s\n", dir->d_ino, dir->d_rec_len,
           dir->d_name_len, dir->d_name);
    return 0;
}

int ls () {
    if (!(curr->in_type&EXT2_TYPE_DIR)) {
        test_errprintf("not a directory\n");
        return 1;
    }

    ext2_iter_dir(curr, print_dir, &info);
	return 0;
}

void do_touch(char* s) {
    if (!ext2_touch(curr_ino, s, 0640, &info))
        test_errprintf("fail\n");
}

void print_stat() {
    print_inode(curr);
}

void do_cd(const char* s) {
    uint32_t ino = ext2_lookup_dir(curr, s, &info);
    struct ext2_inode *inode = ext2_get_inode(ino, &info);
    if (!inode) {
        printf("%s dont exists\n", s);
        return;
    }

    if (!(inode->in_type&EXT2_TYPE_DIR)) {
        printf("%s is not a directory\n", s);
        return;
    }

    curr_ino = ino;
    curr = inode;
}

bool do_mkdir(char* s) {
	if (!ext2_mkdir(curr_ino, s, 0640, &info)) {
		test_errprintf("fail\n");
		return false;
	}
	return true;
}

void* save_area(size_t* sz) {
	*sz = info.sp->s_blocks_count * info.block_size;
	return info.sp;
}

void dump() {
    printf("superblock size : %ld\n", sizeof(struct ext2_superblock));
    printf("inode count : %d\n", info.sp->s_inodes_count);
    printf("block count : %d\n", info.sp->s_blocks_count);

    printf("block size  : %d\n", info.block_size);
    printf("frag  size  : %d\n", ext2_frag_size(info.sp));
    printf("blocks per group : %d\n", info.sp->s_blocks_per_group);
    printf("inodes per group : %d\n", info.sp->s_inodes_per_group);
    printf("number of block groups : %d\n", info.group_count);

    printf("features : ");
    if (info.sp->s_feature_ro_compat&EXT2_FEATURE_SPARSE_SUPER) {
        printf("sparse file ");
    }

    printf("\n");
    printf("revision level : %d\n", info.sp->s_rev_level);
    printf("inode size : %d\n", info.sp->s_inode_size);

    printf("\n");

    struct ext2_group_desc *bg = info.bg;
    for (size_t i = 0 ; i < info.group_count; i++, bg++) {
        printf("Group %ld:\n", i);
        printf("block bitmap %d\n", bg->g_block_bitmap);
        printf("inode bitmap %d\n", bg->g_inode_bitmap);
        printf("inode table  %d-%d\n", bg->g_inode_table,
               bg->g_inode_table + (info.sp->s_inode_size * info.sp->s_inodes_per_group) / info.block_size - 1);
        printf("%d free blocks, %d free inodes, %d directories\n",
               bg->g_free_blocks_count,
               bg->g_free_inodes_count,
               bg->g_dir_count);
        printf("\n");
    }
}
