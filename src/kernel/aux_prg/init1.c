#include <libc/sys.h>
#include <libc/stdio.h>

int main() {
    r1_prires(3);
    //On est désormais en ring 3

    char msg[] = "Hello world !\n";
    write(1, (uint8_t*)msg, 14);
    printf("printf: Hello world !\n");

    return 0;
}
