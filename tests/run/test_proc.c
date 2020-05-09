#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc < 1) {
        fprintf(stderr, "usage: %s fname\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "w+");

    for (int i = 0;; ++i) {
        fprintf(fp, "coucou %d\n", i);
        sleep(1);
    }

    return 1;
}
