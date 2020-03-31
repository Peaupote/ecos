#include <stdio.h>

#include "../tutil.h"
#include "import-c.h"

void sq_test_strlen() {
	tAssert(my_strlen("") == 0);
    tAssert(my_strlen("a") == 1);
    tAssert(my_strlen("aaaaaaaaa"));
}

void sq_test_memcmp() {
    char *t1 = "abc",
        *t2 = "aaa";

	tAssert(!my_memcmp(t1, t2, 0));
    tAssert(!my_memcmp(t1, t2, 1));
    tAssert(my_memcmp(t1, t2, 3) > 0);
    tAssert(my_memcmp(t2, t1, 3) < 0);
}

int main(void) {
    test_init("libc string.h\n");
	sq_test_strlen();
	sq_test_memcmp();
    return 0;
}
