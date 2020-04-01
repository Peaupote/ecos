#include <libc/sys.h>
#include <libc/stdio.h>

int main() {
    r1_prires(3);
    //On est désormais en ring 3

    printf("Hello world !\n");

	return 0;
}
