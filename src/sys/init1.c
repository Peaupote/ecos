#include <unistd.h>
#include <libc/sys.h>

int main() {
	pid_t mpid = getpid();
	r1_ttyown(mpid);//TODO: handle sh exit

    r1_prires(3);
    //On est désormais en ring 3
	setpriority(-15);

	const char *args[1] = {0};
	const char  *env[1] = {0};

	execve("/home/bin/sh", args, env);

    return 0;
}
