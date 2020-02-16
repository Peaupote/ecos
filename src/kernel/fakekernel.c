/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__)
#error "You are not using a cross-compiler"
#endif

#if !defined(__i386__)
#error "Needs to be compiled with a ix86-elf compiler"
#endif

#include "../def.h"

#include "../util/vga.h"
#include "../util/string.h"

#ifdef BDI
extern uint8_t boot_debug_out[256];
#endif


uint8_t test_paging_post(void)
{
	/*adresses virtuelles*/
	volatile uint8_t* test_good1 = (uint8_t*)0x003fffff;
	volatile uint8_t* test_good2 = (uint8_t*)0x00000001;
	volatile uint8_t* test_good3 = (uint8_t*)0x00001001;
	volatile char t_1 = *test_good1 == 42;
	volatile char t_2 = *test_good2 == 42;
	volatile char t_3 = 0;
	volatile char t_4 = 0, t_5 = 0;
	t_3 = *test_good3 == 42;
	*test_good2 = 57;
	t_4 = *test_good2 == 57;
	t_5 = *test_good3 == 57;
	return t_1 && t_2 && t_3 && t_4 && t_5;
	//uint8_t test_bad = *((uint8_t*)0x00400000);
	//return test_bad == 57;
}

void kernel_main(void)
{
	char msg0[] = "Hello kernel world\n";
#ifdef TEST_PAGING
	char msg1[] = "\ttest_0: \n";
#endif
#ifdef BDI
	char msg2[] = "\tboot cr0: _._._._.\n";
	char msg3[] = "\tcur  cr0: _._._._.\n";
	char msg4[] = "\t._ -- ._\n";
#endif

    /* Initialize terminal interface */
    terminal_initialize();

    terminal_writestring(msg0);
#ifdef TEST_PAGING
	msg1[8] = test_paging_post() ? '1' : '0';
    terminal_writestring(msg1);
#endif
#ifdef BDI
	int32_to_str_hexa(msg2 + 11, *(uint32_t*)boot_debug_out);
	int32_to_str_hexa(msg3 + 11, *(uint32_t*)(boot_debug_out+4));
	int8_to_str_hexa(msg4 + 1, *(uint8_t*)(boot_debug_out+8));
	int8_to_str_hexa(msg4 + 7, *(uint8_t*)(boot_debug_out+9));
    
	terminal_writestring(msg2);
    terminal_writestring(msg3);
    terminal_writestring(msg4);
#endif
}
