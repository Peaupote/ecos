#include "../int.h"
#include "../sys.h"

uint32_t counter = 0;
char     is_sleep = 0;

void sleep_hdl(uint32_t time) {
    counter = time;
    is_sleep = 1;
}

void end_sleep(void) {
    is_sleep = 0;

    // sleep call itself for test purposes
    int_syscall(SLEEP);
}
