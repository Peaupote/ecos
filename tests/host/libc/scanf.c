#include <stdio.h>
#include <string.h>

#include "../tutil.h"
#include "import-c.h"

void sq_test_scanf() {
	int  r_ints[5];
	unsigned r_unsi;
	char r_str1[100], r_str2[100], r_str3[100];
	char cr;
	uint64_t r_u64;
	long int r_i64;

	tAssert(my_sscanf("42ecos ecos2  abC857DeF string", "%d%s%s%p%3s",
				r_ints, r_str1, r_str2, &r_u64, &r_str3) == 5);
	tAssert(r_ints[0] == 42);
	tAssert(!strcmp(r_str1, "ecos"));
	tAssert(!strcmp(r_str2, "ecos2"));
	tAssert(r_u64 == 0xabc857def);
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

	tAssert(my_sscanf("2147483647", "%d", r_ints) == 1);
	tAssert(r_ints[0] == 2147483647);
	tAssert(my_sscanf("2147483648", "%d", r_ints) == 0);
	tAssert(my_sscanf("-2147483648", "%d", r_ints) == 1);
	tAssert(r_ints[0] == -2147483648);
	tAssert(my_sscanf("-2147483649", "%d", r_ints) == 0);
	
	tAssert(my_sscanf("4294967295", "%u", &r_unsi) == 1);
	tAssert(r_unsi == 4294967295);
	tAssert(my_sscanf("4294967296", "%u", &r_unsi) == 0);
	
	tAssert(my_sscanf("9223372036854775807", "%ld", &r_i64) == 1);
	tAssert(r_i64 == 9223372036854775807);
	tAssert(my_sscanf("9223372036854775808", "%ld", &r_i64) == 0);
	tAssert(my_sscanf("-9223372036854775808", "%ld", &r_i64) == 1);
	tAssert(r_i64 == (long)(((unsigned long)1L) << 63));
	tAssert(my_sscanf("-9223372036854775809", "%ld", &r_i64) == 0);
	
	tAssert(my_sscanf("18446744073709551615", "%lu", &r_u64) == 1);
	tAssert(r_u64 == ~(long unsigned)0L);
	tAssert(my_sscanf("18446744073709551616", "%lu", &r_u64) == 0);
}

int main(void) {
    test_init("libc scanf\n");
	sq_test_scanf();
    return 0;
}
