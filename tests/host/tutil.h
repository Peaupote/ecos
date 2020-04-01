#ifndef _TUTIL_H
#define _TUTIL_H

#include <util/test.h>

#include <stddef.h>
#include <stdint.h>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define kAssert(P) tassert(P, __FILE__":"STR(__LINE__)" "#P)
#define tAssert(P) tassert(P, __FILE__":"STR(__LINE__)" "#P)
#define kassert(...) tassert(__VA_ARGS__)

void test_init(const char *n);
int rand_rng(int min, int max);
uint64_t rand64();
void rand_perm(size_t sz, size_t* dst);

void test_fail(const char *n);
void texit(const char *n);
void test_infoi(const char *fmt, int i);

enum klog_level {
    Log_error, Log_warn, Log_info, Log_verb
};

void kpanic(const char* p);
void kpanic_ct(const char* p);
void tassert(uint8_t, const char *msg);
void klog (enum klog_level lvl, const char *head, const char *msg);
void klogf(enum klog_level lvl, const char *head, const char *msgf, ...);

#endif
