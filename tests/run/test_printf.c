#include <stdio.h>
#include <assert.h>

// test bad format

int main() {
    int rc;

    rc = printf ("%", 1);
    assert(rc < 0);
    perror("printf");

    rc = printf("%12", 1);
    assert(rc < 0);
    perror("printf");

    for (int i = 0; i < 100; ++i) {
        printf("bonjour");
    }

    return 0;
}
