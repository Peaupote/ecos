#include <stdio.h>
#include <string.h>

#include "../tutil.h"
#include "import-c.h"

void sq_test_scanf() {
	int  r_ints[5];
	char r_str1[100], r_str2[100], r_str3[100];
	char cr;
	uint64_t r_64;

	tAssert(my_sscanf("42ecos ecos2  abC857DeF string", "%d%s%s%p%3s",
				r_ints, r_str1, r_str2, &r_64, &r_str3) == 5);
	tAssert(r_ints[0] == 42);
	tAssert(!strcmp(r_str1, "ecos"));
	tAssert(!strcmp(r_str2, "ecos2"));
	tAssert(r_64 == 0xabc857def);
	tAssert(!strcmp(r_str3, "str"));

	tAssert(my_sscanf("42ecos ecos2  abC857DeF string", "%*d%s%*s%*p%3s",
				r_str1, &r_str3) == 2);
	tAssert(!strcmp(r_str1, "ecos"));
	tAssert(!strcmp(r_str3, "str"));

	tAssert(my_sscanf("e ecos2 ecos3 ecos4", "%c%6c %5c %10c",
				&cr, r_str1, r_str2, r_str3) == 4);
	tAssert(cr == 'e');
	tAssert(!strncmp(r_str1, " ecos2", 6));
	tAssert(!strncmp(r_str2, "ecos3", 5));
	tAssert(!strncmp(r_str3, "ecos4", 5));
	
	tAssert(my_sscanf("0x42 0X43 44 045 0", "%i%i%i%i%i",
				r_ints, r_ints+1, r_ints+2, r_ints+3, r_ints+4) == 5);
	tAssert(r_ints[0] == 0x42);
	tAssert(r_ints[1] == 0x43);
	tAssert(r_ints[2] ==   44);
	tAssert(r_ints[3] ==  045);
	tAssert(r_ints[4] ==    0);
	
	tAssert(my_sscanf("-0x42 48 -43 -044", "%i%*i%i%i",
				r_ints, r_ints+1, r_ints+2) == 3);
	tAssert(r_ints[0] == -0x42);
	tAssert(r_ints[1] ==   -43);
	tAssert(r_ints[2] ==  -044);
}

int main(void) {
    test_init("libc scanf\n");
	sq_test_scanf();
    return 0;
}
