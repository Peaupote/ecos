#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define NB_TEST 5

int test_0(int ac __attribute__((unused)),
		char** av __attribute__((unused)) ) {
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

int test_1(int ac __attribute__((unused)),
			char** av __attribute__((unused)) ) {
	int* p = NULL;
	*p = 42;
	return 1;
}

int test_2(int ac, char** av) {
	long long int itv = 50000000;
	if (ac < 3 || (ac >= 4 && (sscanf(av[3], "%lld", &itv) != 1 || itv <= 0)))
		return 2;
	unsigned a = 0, b = 1;
	long long int r = itv;
	while (1) {
		b = a + b;
		a = b - a;
		if (!--r) {
			r = itv;
			printf("%s: %11d\n", av[2], a);
		}
	}
}

int test_3(int ac, char** av) {
	int it, max;
	if (ac < 4 || sscanf(av[2], "%d", &it)  != 1
			   || sscanf(av[3], "%d", &max) != 1)
		return 2;
	for (; it < max; ++it) {
		printf("%d\n", it);
		usleep(20000L);
	}
	return 0;
}

int test_4(int ac, char**av) {
	int nb;
	if (ac < 3 || sscanf(av[2], "%d", &nb) != 1) return 2;
	for (int i = 0; i < nb; ++i)
		printf("line %d\n", i);
	return 0;
}

typedef int (*test_t)(int argc, char** argv);
test_t tests[NB_TEST] = {test_0, test_1, test_2, test_3, test_4};

int main(int argc, char* argv[]) {
	if (argc < 2) return 2;
	int testnum;
	if (sscanf(argv[1], "%d", &testnum) != 1
			|| testnum < 0 || testnum >= NB_TEST) {
		printf("Argument incorrect: '%s'\n", argv[1]);
		return 2;
	}
	return tests[testnum](argc, argv);
}
