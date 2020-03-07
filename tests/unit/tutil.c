#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void test_init(const char *n){
	unsigned sd = time(0);
	printf("TEST %s seed=%x\n", n, sd);
	srand(sd);
}

int rand_rng(int min, int max) {
	return min + (rand() % (max - min + 1));
}

void test_fail(const char *n) {
	printf("ECHEC %s\n", n);
}

void test_infoi(const char *fmt, int i) {
	printf(fmt, i);
}
