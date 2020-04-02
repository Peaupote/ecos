#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	for (int argi = 1; argi < argc; ++argi) {
		if (argi > 1) printf(" ");
		printf("%s", argv[argi]);
	}

    printf("error\n");
    exit(1);

    return 0;
}
