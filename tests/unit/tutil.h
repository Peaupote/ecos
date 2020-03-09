#ifndef _TUTIL_H
#define _TUTIL_H

#define TEST_UNIT

#include <stddef.h>
#include <stdint.h>

void test_init(const char *n);
int rand_rng(int min, int max);
uint64_t rand64();
void test_fail(const char *n);
void texit(const char *n);
void test_infoi(const char *fmt, int i);

#endif
