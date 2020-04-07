#include <sys/stat.h>
#include <stdio.h>

int main (int argc, char *argv[]) {
    if (argc == 1) {
        printf("usage: %s files\n", argv[0]);
        return 1;
    }

    struct stat st;
    for (int i = 1; i < argc; i++) {
        if (stat(argv[i], &st) < 0) {
            printf("error on %s\n", argv[i]);
        } else {
            printf("File: %s\n", argv[i]);
            printf("Size: %d\t", st.st_size);
            printf("Blocks: %d\t", st.st_blocks);
            printf("IO Blocks: %d\t", st.st_blksize);
            switch(st.st_mode&0xf000) {
            case TYPE_REG: printf("regular file\n"); break;
            case TYPE_DIR: printf("directory\n"); break;
            default: printf("unknown type %x\n", st.st_mode&0xf000);break;
            }
            printf("Device: %d\t", st.st_dev);
            printf("Inode: %d\t", st.st_ino);
            printf("Links: %d\n", st.st_nlink);
        }
    }

    return 0;
}
