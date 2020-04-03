#include <util/test.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "ext2_wrap.h"

static char *line = NULL;

void touch() {
    if (!is_curr_dir()) {
        fprintf(stderr, "not in a dir\n");
        return;
    }

    char *s = strtok(line, " \n");
    if (!(s = strtok(0, " \n"))) {
        fprintf(stderr, "usage: touch file\n");
        return;
    }

    do_touch(s);
}

void cd () {
    char *s1 = strchr(line, ' ');
    if (!s1) {
        printf("usage: cd dir\n");
        return;
    }

    char *s2 = strchr(++s1, ' ');
    if (!s2) {
        s2 = strchr(s1, '\n');
        if (!s2) s2 = line + strlen(line);
    }

    *s2 = 0;
    do_cd(s1);
}

void cmd_stat() {
    char *s = strtok(line, " \n");

    for (s = strtok(0, " \n"); s; s = strtok(0, " \n"))
        print_stat(s);
}

void cmd_mkdir() {
    char *s = strtok(line, " ");

    for (s = strtok(0, " "); s; s = strtok(0, " "))
        if (!do_mkdir(s))
            return;
}

void save() {
    char *s = strtok(line, " \n");
    s = strtok(0, " \n");
    if (!s) {
        printf("usage: save name\n");
        return;
    }

    int fd = open(s, O_WRONLY|O_CREAT, 0640);
    if (fd < 0) { perror("open"); goto end; }

    if (lseek(fd, 1024, SEEK_SET) < 0) { perror("seek"); goto end; }
    size_t sz;
    void*  sp = save_area(&sz);
    if (write(fd, sp, sz) < 0) {
        perror("write");
        goto end;
    }

    printf("fs saved in %s\n", s);

end:
    close(fd);
}

int main(int argc, char *argv[]) {
    int rtst = 0;

    if (argc < 2) {
        fprintf(stderr, "usage: %s part.img\n", argv[0]);
        exit(1);
    }

    int rc, fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    // load integrality of file in ram
    rc = lseek(fd, 0, SEEK_END);
    if (rc < 0) {
        perror("seek");
        exit(1);
    }

    void *fs = malloc(rc);
    if (!fs) {
        perror("malloc");
        close(fd);
        exit(1);
    }

    if (lseek(fd, 0, SEEK_SET) < 0) {
        perror("seek");
        close(fd);
        free(fs);
        exit(1);
    }

    while(rc > 0) {
        int rt = read(fd, fs, rc);
        if(rt <= 0) {
            perror("read");
            close(fd);
            free(fs);
            exit(1);
        }
        rc -= rt;
    }

    close(fd);

    int rt = init_ext2(fs);
    if (rt) {
        fprintf(stderr, "can't mount\n");
        rtst = 1;
        goto exit_main;
    }

    size_t len = 0;
    ssize_t nread;
    while (1) {
        printf("> ");
        if ((nread = getline(&line, &len, stdin)) < 0) {
            rtst = 1;
            goto exit_main;
        }

        if (!strncmp(line, "ls", 2)) {
            if(ls()) {
                rtst = 1;
                goto exit_main;
            }
        }
        else if (!strncmp(line, "cd", 2)) cd();
        else if (!strncmp(line, "stat", 4)) cmd_stat();
        else if (!strncmp(line, "mkdir", 5)) cmd_mkdir();
        else if (!strncmp(line, "save", 4)) save();
        else if (!strncmp(line, "dump", 4)) dump();
        else if (!strncmp(line, "touch", 4)) touch();
        else if (!strncmp(line, "\n", 1)) {}
        else {
            printf("unkonwn command %s", line);
        }
    }

exit_main:
    free(line);
    free(fs);
    return rtst;
}
