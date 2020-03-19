#define TEST

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <fs/ext2.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s part.img\n", argv[0]);
        exit(1);
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    int rc;
    struct ext2_superblock sp;

    rc = lseek(fd, 1024, SEEK_SET);
    if (rc < 0) {
        perror("seek");
        exit(1);
    }

    rc = read(fd, &sp, sizeof(struct ext2_superblock));
    if (rc < 0) {
        perror("read");
        exit(1);
    }

    printf("inode count : %d\n", sp.s_inodes_count);
    printf("block count : %d\n", sp.s_blocks_count);
    printf("block size  : %d\n", 1024 << sp.s_log_block_size);
    printf("frag size   : %d\n", 1024 << sp.s_log_frag_size);

    close(fd);
    return 0;
}
