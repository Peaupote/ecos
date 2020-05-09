/**
 * Test file for concurrent access on same file
 */

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>

void test_read() {
    printf("Test concurrent reads\n");

    int fd = open("/home/test/test.c", O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    pid_t f = fork();
    pid_t pid = getpid();

    int pos = 0, rc;
    char buf[64];
    while ((rc = read(fd, buf, 64)) > 0) {
        pos += rc;
        printf("[pid %d] read %d (size readed %d).\n", pid, rc, pos);
    }

    printf("[pid %d] done reading (total file size %d).\n", pid, pos);
    close(fd);
    if (!f) exit(0);
    wait(0);
}

void test_write() {
    printf("Test concurrent writes\n");

    int fd = open("/home/test/file", O_WRONLY|O_CREAT|O_TRUNC);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    pid_t f = fork();
    pid_t pid = getpid();

    int pos = 0, rc;
    char a = f ? 'a' : 'b';
    while (pos < 42 && (rc = write(fd, &a, 1)) > 0) {
        pos += rc;
        printf("[pid %d] write %c.\n", pid, a);
    }

    printf("[pid %d] done.\n", pid);
    close(fd);

    if (!f) exit(0);
    wait(0);

    fd = open("/home/test/file", O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    char buf[85] = { 0 };
    while ((rc = read(fd, buf, 84)) > 0);

    printf("generated random file:\n %s\n", buf);
    close(fd);
}

void test_rdwr() {
    printf("Test concurrent read and writes\n");

    pid_t f = fork();
    pid_t pid = getpid();

    /* if (f) sleep(1); */

    int rc;
    int fd = open("/home/test/file", O_RDWR|O_CREAT|O_TRUNC);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    if (!f) { // read and move forward in file
        char buf[16] = { 0 };
        for (int i = 0; i < 20; ++i) {
            rc = read(fd, buf, 15);
            if (rc < 0) {
                perror("read");
                close(fd);
                exit(0);
            }

            buf[rc] = 0;
            printf("[pid %d] %d: read %d '%s'\n", pid, i, rc, buf);
            sleep(3);
        }

        close(fd);
        exit(0);
    }

    // writes 3 times
    for (int i = 0; i < 3; ++i) {
        rc = write(fd, "something", 9);
        if (rc <= 0) {
            perror("write");
            close(fd);
            exit(1);
        }

        struct stat st;
        fstat(fd, &st);
        printf("[pid %d] %d: write 'something' (file size %d)\n",
               pid, i, st.st_size);
        sleep(5);
    }

    close(fd);

    printf("set file size to 0 (must invalidate next reads)\n");
    fd = open("/home/test/file", O_RDWR|O_TRUNC);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    sleep(5);

    printf("fill file again\n");
    for (int i = 0; i < 3; ++i) {
        rc = write(fd, "something", 9);
        if (rc <= 0) {
            perror("write");
            close(fd);
            exit(1);
        }

        struct stat st;
        fstat(fd, &st);
        printf("[pid %d] %d: write 'something' (file size %d)\n",
               pid, i, st.st_size);
        sleep(5);
    }

    close(fd);
    wait(0);
}

void usage(char *name) {
    printf("usage: %s test_id\n", name);
    printf("\t1  test concurrent reads on same channel\n");
    printf("\t2  test concurrent writes on same channel\n");
    printf("\t3  test concurrent writes and reads on differents channels\n");
}

#define NTEST 3
typedef void (*test_func)(void);
test_func test_funcs[NTEST] = { test_read, test_write, test_rdwr };

int main(int argc, char *argv[]) {
    if (argc == 1) {
        usage(argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        int n = atoi(argv[i]);
        if (0 < n && n <= NTEST) test_funcs[n-1]();
        else {
            printf("error: invalid test %d\n", n);
            usage(argv[0]);
            return 1;
        }
    }

    return 0;
}
