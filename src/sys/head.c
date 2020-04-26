#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    char buf[1024];
    FILE *fp = stdin;
    int i = 0, n = 0;

    char *lineptr = 0;
    size_t bufsize = 0;
    ssize_t len = 0;

    if (argc == 1) goto start;

    for (i = 1; i < argc; i++) {
        fp = fopen(argv[i], "r");
        if (!fp) {
            sprintf(buf, "head: %s", argv[i]);
            perror(buf);
            continue;
        }

    start:
        n = 0;
        while (n++ < 10 && (len = getline(&lineptr, &bufsize, fp)) > 0) {
            fputs(lineptr, stdout);
            fflush(stdout);
        }

        fclose(fp);
    }

    free(lineptr);

    return 0;
}
