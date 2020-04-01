#include "tutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void test_init(const char *n){
#ifndef TEST_SEED
	unsigned sd = time(0);
#else
	unsigned sd = TEST_SEED;
#endif
	printf("TEST %s seed=%x\n", n, sd);
	srand(sd);
}

int rand_rng(int min, int max) {
	return min + (rand() % (max - min + 1));
}

void rand_perm(size_t sz, size_t* dst) {
	for (size_t i = 0; i < sz; ++i) {
		size_t j = rand_rng(0, i);
		dst[i] = dst[j];
		dst[j] = i;
	}
}

uint64_t rand64() {
	uint64_t p1 = rand();
	uint64_t p2 = rand();
	uint64_t p3 = rand();
	uint64_t p4 = rand();
	uint64_t p5 = rand();
	return p1 ^ (p2<<15) ^ (p3<<30) ^ (p4<<45) ^ (p5<<60);
}

void test_fail(const char *n) {
	printf("ECHEC %s\n", n);
}
void texit(const char *n) {
	test_fail(n);
	exit(1);
}

void test_infoi(const char *fmt, int i) {
	printf(fmt, i);
}

void kpanic(const char *p) {
	texit(p);
}
void kpanic_ct(const char *p) {
	texit(p);
}
void tassert(uint8_t b, const char *p) {
	if (!b) kpanic(p);
}
void klog (enum klog_level lvl __attribute__((unused)),
		const char *head __attribute__((unused)),
		const char *msg __attribute__((unused))) {
}
void klogf(enum klog_level lvl __attribute__((unused)),
		const char *head __attribute__((unused)),
		const char *msgf __attribute__((unused)), ...) {
}
