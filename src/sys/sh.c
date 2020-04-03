#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys.h>

int main() {
    printf("ecos-shell version 0.1\n");

    int rc;
    char *ptr, line[258];
    const char **aptr;
    const char *args[256] = { 0 };
    const char *env[1] = { 0 };

    while(1) {
        rc = read(0, line, 256);//TODO: attendre \n
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

        rc = fork();
        if (rc < 0) {
            printf("error: fork\n");
        } else if (rc == 0) {
            rc = execve(args[0], args, env);
            printf("error: %d ...\n", rc);
            exit(1);
        }

        int rs;
        wait(&rs);
        printf("process %d exited with status %d\n", rc, rs);
    }
}