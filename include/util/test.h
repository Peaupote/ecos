#ifndef _U_TEST_H
#define _U_TEST_H

#ifdef __is_test_unit
#define TEST_U(N) test_fun_##N
#else

#ifdef __is_test
#define TEST_D(N) test_fun_##N
#define TEST_U(N) non_used_##N
#define TEST_O(N) non_used_##N
#else
#define TEST_D(N) N
#define TEST_U(N) N
#define TEST_O(N) N
#endif

#endif

#endif
