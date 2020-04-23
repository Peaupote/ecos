#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define NB_TEST 2

int test_0() {
    pid_t pid;
    int fds[2];
    int rc = pipe(fds);
    if (rc < 0) {
        perror("pipe");
        exit(1);
    }

    if (fork() == 0) {
        char buf[1024];
        pid = getpid();
        close(fds[1]);

        while ((rc = read(fds[0], buf, 1024)) > 0) {
            buf[rc] = 0;
            printf("[pid %d] %s", pid, buf);
        }

        if (rc < 0) {
            perror("write");
            exit(1);
        }

        close(fds[0]);
        exit(0);
    }

    char *str = "Hello !\n";
    int len = strlen(str);
    pid = getpid();
    close(fds[0]);

    while((rc = write(fds[1], str, len)) > 0) {
        sleep(2);
        printf("[pid %d] write \"%s\"", pid, str);
    }

    close(fds[1]);
    return 0;
}

int test_1() {
	int* p = NULL;
	*p = 42;
	return 1;
}

typedef int (*test_t)(void);
test_t tests[NB_TEST] = {test_0, test_1};

int main(int argc, char* argv[]) {
	for (int i = 1; i < argc; ++i) {
		int testnum;
		if (sscanf(argv[i], "%d", &testnum) != 1
				|| testnum < 0 || testnum >= NB_TEST)
			printf("Argument incorrect: '%s'\n", argv[i]);
		else {
			int rt = tests[testnum]();
			if (rt) return rt;
		}
	}
	return 0;
}
