#ifndef _U_TEST_H
#define _U_TEST_H

#ifdef __is_test
int printf(const char* format, ...);
#define test_printf(...) printf(__VA_ARGS__)
#define test_errprintf(F, ...) printf(("ERR: " F),## __VA_ARGS__)
#else
#define test_printf(...)
#define test_errprintf(...)
#endif

#ifdef __is_test_unit
#define TEST_U(N) test_fun_##N
#else
#define TEST_U(N) N
#endif

#endif
