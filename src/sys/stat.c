#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// by default don't follow symlink
typedef int stat_func(const char *fname, struct stat *st);
stat_func *func = lstat;

int main (int argc, char *argv[]) {
    if (argc == 1) {
        printf("usage: %s files\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-L")) func = stat;
    }

    char buf[256] = { 0 };
    struct stat st;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') continue;
        else if (func(argv[i], &st) < 0) {
            sprintf(buf, "stat: %s", argv[i]);
            perror(buf);
            return 1;
        } else {
            printf("File: %s", argv[i]);
            if ((st.st_mode&0xf000) == TYPE_SYM) {
                int rc = readlink(argv[i], buf, 255);
                if (rc < 0) {
                    sprintf(buf, "stat: %s: readlink", argv[i]);
                    perror(buf);
                    return 1;
                }

                buf[rc] = 0;
                printf(" -> %s", buf);
            }
            printf("\n");

            printf("Size: %d\t", st.st_size);
            printf("Blocks: %d\t", st.st_blocks);
            printf("IO Blocks: %d\t", st.st_blksize);
            switch(st.st_mode&0xf000) {
            case TYPE_REG:  printf("regular file\n");     break;
            case TYPE_DIR:  printf("directory\n");        break;
            case TYPE_SYM:  printf("symbolic link\n");    break;
            case TYPE_CHAR: printf("character device\n"); break;
            case TYPE_FIFO: printf("fifo\n");             break;
            default: printf("unknown type %x\n", st.st_mode&0xf000);break;
            }
            printf("Device: %d\t", st.st_dev);
            printf("Inode: %d\t", st.st_ino);
            printf("Links: %d\n", st.st_nlink);
        }
    }

    return 0;
}
