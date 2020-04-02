#include <unistd.h>
#include <libc/sys.h>

int main() {
    r1_prires(3);
    //On est désormais en ring 3
	setpriority(-15);

	const char *args[256] = {0};
	const char  *env[ 1 ] = {0};

	execve("/home/bin/sh", args, env);

    return 0;
}
