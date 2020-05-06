#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <libc/sys.h>
#include <stdbool.h>

int main() {
    setenv("HOME", "/home/test", 0);
    setenv("PATH", "/bin",       0);

	chdir("/home/test");

	while (true) {
		int rf = fork();
		if (rf < 0) {
			perror("init1 - fork");
			return 1;
		} else if (!rf) {
			r1_prires(3);
			//On est désormais en ring 3
			setpriority(-15);

			const char arg0[] = "/bin/sh";
			const char arg1[] = "-t";
			const char *args[2] = { arg0, arg1 };
			execv(arg0, args);
			perror("init1 - exec sh");
			return 1;
		}
		r1_ttyown(rf);

		int st;
		while (waitpid(rf, &st) < 0);
		if (st)
			printf("\033\nsh exited with status %d\n", st);
		printf("\033\n\nrestarting sh...\n");
	}

    return 0;
}
