#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 4) goto usage;

    int rc;
    if (argc == 4 && !strcmp(argv[1], "-s")) rc = symlink(argv[2], argv[3]);
    else if (argc == 3) rc = link(argv[1], argv[2]);
    else goto usage;

    if (rc < 0) {
        perror("ln");
        return 1;
    }

    return 0;

usage:
    fprintf(stderr, "usage: %s [-s symlink] target link_name\n", argv[0]);
    return 1;
}
