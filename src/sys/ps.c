#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

int is_not_pid(const char *s) {
    while (*s >= '0' && *s <= '9') ++s;
    return *s;
}

int main () {
    struct dirp *dirp = opendir("/proc");
    struct dirent *dir;
    int pid, count = 0;
    char buf[256];

    printf("PID STATE CMD\n");
    while ((dir = readdir(dirp))) {
        if (is_not_pid(dir->d_name)) continue;

        pid = atoi(dir->d_name);
        sprintf(buf, "/proc/%d/stat", pid);
		FILE* fstat = fopen(buf, "r");
        if (!fstat) {
            perror("ps: open");
            closedir(dirp);
            exit(1);
        }

		int p; char cmd[256]; char st;
		int mc = fscanf(fstat, "%d ", &p);
		if (fgetc(fstat)!='(') {
			mc = 1000;
			goto end_pid;
		}
		char *cmdit = cmd;
		int c;
		do {
			c = fgetc(fstat);
			if (c == EOF) {
				mc = 1000;
				goto end_pid;
			}
			*cmdit++ = c;
		} while (c != ')');
		*(cmdit-1) = '\0';
		mc += fscanf(fstat, " %c", &st);
end_pid:
		fclose(fstat);
        if (mc != 2)
			printf("%-3d #ERROR (%d)\n", pid, mc);
		else
			printf("%-3d %c     %s\n", pid, st, cmd);

        count++;
    }

    printf("total %d\n", count);

    closedir(dirp);
    return 0;
}
