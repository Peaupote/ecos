#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "../tutil.h"
#include "import-c.h"

void sq_test_strcmp() {
    char *buf = "aaaaaaaaaaaa";

    tAssert(my_strcmp("aa", "aa") == 0);
    tAssert(my_strcmp("ab", "aa") > 0);
    tAssert(my_strcmp(buf, "aa") > 0);
    tAssert(my_strcmp("aa", buf) < 0);
}

void sq_test_strncmp() {
    char *buf = "aaaaaaaaaaaa";

    tAssert(my_strncmp("aa", "aa", 3) == 0);
    tAssert(my_strncmp("ab", "aa", 3) > 0);
    tAssert(my_strncmp(buf, "aa", 5) > 0);
    tAssert(my_strncmp("aa", buf, 5) < 0);
    tAssert(my_strncmp("aa", buf, 2) == 0);
}

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
    sq_test_strcmp();
    return 0;
}
