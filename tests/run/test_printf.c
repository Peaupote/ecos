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
	return 0;
}
