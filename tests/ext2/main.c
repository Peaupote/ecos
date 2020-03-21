#define TEST

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <fs/ext2.h>

static char *line = 0;
static struct ext2_mount_info info;
static struct ext2_inode *curr;

static void print_type(uint8_t type) {
    printf("type : ");
    switch (type&0xf000) {
    case EXT2_TYPE_DIR: printf("directory\n"); break;
    case EXT2_TYPE_REG: printf("regular file\n"); break;
    default: printf("other\n"); break;
    }
}

static void print_inode(struct ext2_inode *inode) {
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
    printf("%d (%d) %*s\n", dir->d_ino, dir->d_rec_len,
           dir->d_name_len, dir->d_name);
    return 0;
}

void ls () {
    if (!(curr->in_type&EXT2_TYPE_DIR)) {
        fprintf(stderr, "not a directory\n");
        exit(1);
    }

    ext2_iter_dir(curr, print_dir, &info);
}

void print_stat() {
    print_inode(curr);
}

void cd () {
    char *s1 = strchr(line, ' ');
    if (!s1) {
        printf("usage: cd [dir]\n");
        return;
    }

    char *s2 = strchr(++s1, ' ');
    if (!s2) {
        s2 = strchr(s1, '\n');
        if (!s2) s2 = line + strlen(line);
    }

    *s2 = 0;

    struct ext2_inode *inode = ext2_lookup_dir(curr, s1, &info);
    if (!inode) {
        printf("%s dont exists\n", s1);
        return;
    }

    if (!(inode->in_type&EXT2_TYPE_DIR)) {
        printf("%s is not a directory\n", s1);
        return;
    }

    curr = inode;
}

void cmd_mkdir() {
    char *s = strtok(line, " ");

    for (s = strtok(0, " "); s; s = strtok(0, " ")) {
        ext2_mkdir(curr, s, &info);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s part.img\n", argv[0]);
        exit(1);
    }

    int rc, fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    // load integrality of file in ram
    rc = lseek(fd, 0, SEEK_END);
    if (rc < 0) {
        perror("seek");
        exit(1);
    }

    void *fs = malloc(rc);
    if (!fs) {
        perror("malloc");
        exit(1);
    }

    if (lseek(fd, 0, SEEK_SET) < 0) {
        perror("seek");
        exit(1);
    }

    rc = read(fd, fs, rc);
    if (rc < 0) {
        perror("write");
        exit(1);
    }

    close(fd);

    rc = ext2_mount(fs, &info);
    if (rc < 0) {
        fprintf(stderr, "can't mount\n");
        exit(1);
    }

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

    curr = ext2_find_inode(EXT2_ROOT_INO, &info); // root

    size_t len = 0;
    ssize_t nread;
    while (1) {
        printf("> ");
        if ((nread = getline(&line, &len, stdin)) < 0) {
            exit(1);
        }

        if (!strncmp(line, "ls", 2)) ls();
        else if (!strncmp(line, "cd", 2)) cd(line);
        else if (!strncmp(line, "stat", 4)) print_stat();
        else if (!strncmp(line, "mkdir", 5)) cmd_mkdir();
        else if (!strncmp(line, "\n", 1)) {}
        else {
            printf("unkonwn command %s", line);
        }
    }

    free(line);
    free(fs);
    return 0;
}
