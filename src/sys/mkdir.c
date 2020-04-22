#include <sys/stat.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("usage: %s dirs...\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        if (mkdir(argv[i], 0640) < 0) {
            char errbuf[256];
            sprintf(errbuf, "mkdir: %s", argv[i]);
            perror(errbuf);
            return 1;
        }
    }

    return 0;
}
