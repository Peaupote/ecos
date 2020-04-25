#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    char buf[1024];
    FILE *fp = stdin;
    size_t rc;

    if (argc == 1) goto start;

    for (int i = 1; i < argc; i++) {
        fp = fopen(argv[i], "r");
        if (!fp) {
            sprintf(buf, "cat: %s", argv[i]);
            perror(buf);
            continue;
        }

    start:
        while ((rc = fread(buf, 1, 1024, fp)) > 0)
            fwrite(buf, 1, rc, stdout);

        fflush(stdout);
        fclose(fp);
    }

    return 0;
}
