#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys.h>
#include <signal.h>

pid_t fg_proc_pid = PID_NONE;

void int_handler(int signum) {
	if (signum != SIGINT) {
		printf("bas signal: %d\n", signum);
		return;
	}
	if (kill(fg_proc_pid, SIGINT))
		printf("error sending SIGINT to %d\n", fg_proc_pid);
}

int main() {
    printf("ecos-shell version 0.1\n");
    
	int rc;
    char *ptr, line[258];
    const char **aptr;
    const char *args[256] = { 0 };
    const char *env[1] = { 0 };

    while(1) {
        rc = read(0, line, 256);//TODO: attendre \n + ne pas aller plus loin
        if (rc < 0) {
            printf("an error occurred\n");
            exit(1);
        }

        line[rc] = '\n';
        line[rc + 1] = 0;

        aptr = args;
        memset(args, 0, 256);
        for (ptr = strtok(line, " \n"); ptr; ptr = strtok(NULL, " \n"))
            *aptr++ = ptr;
        if (aptr == args)
            continue; // commande vide
        *aptr = NULL;

        if (!strcmp(args[0], "exit")) {
            int code = 0;
            if (args[1]) code = atoi(args[1]);
            exit(code);
        }

        rc = fork();
        if (rc < 0) {
            printf("error: fork\n");
        } else if (rc == 0) {
            rc = execve(args[0], args, env);
            printf("error: %d ...\n", rc);
            exit(1);
        }

		fg_proc_pid = rc;
		sighandler_t prev_hnd = signal(SIGINT, &int_handler);
        int rs;
		while (!~wait(&rs));
		signal(SIGINT, prev_hnd);
		fg_proc_pid = PID_NONE;

        printf("process %d exited with status %x\n", rc, rs);
    }
}
