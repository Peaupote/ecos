#include <stdio.h>
#include <string.h>

#include "../tutil.h"
#include "import-c.h"

void sq_test_scanf() {
	int r_int;
	char r_str1[100], r_str2[100];
	uint64_t r_64;
	tAssert(my_sscanf("42ecos ecos2  abC857DeF", "%d%s%s%p", &r_int, r_str1,
				r_str2, &r_64) == 4);
	tAssert(r_int == 42);
	tAssert(!strcmp(r_str1, "ecos"));
	tAssert(!strcmp(r_str2, "ecos2"));
	tAssert(r_64 == 0xabc857def);
}

int main(void) {
    test_init("libc scanf\n");
	sq_test_scanf();
    return 0;
}
