#ifndef _H_LIBC_ASSERT
#define _H_LIBC_ASSERT

#define assert(P) print_assert(P, __FILE__, __FUNCTION__, __LINE__);
void print_assert(int p, const char *file, const char *func, int line);

#endif
