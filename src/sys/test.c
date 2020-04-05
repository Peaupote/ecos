#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

int main() {
    size_t i;
    for (i = 0; i < 1; i++) {
        if (!fork()) break;
    }

    printf("coucou %d\n", i);
    return 0;
}
