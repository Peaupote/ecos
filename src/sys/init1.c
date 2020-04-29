#include <unistd.h>
#include <stdlib.h>
#include <libc/sys.h>

int main() {
    pid_t mpid = getpid();
    r1_ttyown(mpid);//TODO: handle sh exit

    r1_prires(3);
    //On est désormais en ring 3
    setpriority(-15);

    const char *args[1] = { 0 };

    setenv("HOME", "/home/test", 0);
    setenv("PATH", "/home/bin", 0);

    execv("/home/bin/sh", args);

    return 0;
}
