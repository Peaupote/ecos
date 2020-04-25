#include <stdio.h>

int main (int argc, char *argv[]) {
    if (argc == 1) {
        fprintf(stderr, "usage: %s file\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    char buf[16] = { 0 };
    size_t sz;

    printf("read %s\n", argv[1]);
    while ((sz = fread(buf, 1, 15, fp)) > 0) {
        fwrite(buf, 1, sz, stdout);
    }

    fflush(stdout);
    printf("Bye !");

    fclose(fp);

    return 0;
}
