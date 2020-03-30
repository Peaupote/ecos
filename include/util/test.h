#ifndef _U_TEST_H
#define _U_TEST_H

#ifdef __is_test_unit
#define TEST_U(N) test_fun_##N
#else
#define TEST_U(N) N
#endif

#endif
