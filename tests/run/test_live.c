#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>

#include <headers/tty.h>

bool get_live(tty_live_t* rt) {
	int c;
	while ((c = fgetc(stdin)) != TTY_LIVE_MAGIC)
		if (c == EOF) return false;
	char* buf = (char*)rt;
	buf[0] = TTY_LIVE_MAGIC;
	for (size_t i = 1; i < sizeof(tty_live_t); ++i) {
		if ((c = fgetc(stdin)) == EOF) return false;
		buf[i] = c;
	}
	return true;
}

int main() {
	write(STDOUT_FILENO, "\033i", 2);
	tty_live_t lt;
	do {
		if (!get_live(&lt)) return 1;
	} while (lt.ev.key || lt.ev.ascii != 'i');

	unsigned w = 0, h = 0;
	while (get_live(&lt) && !lt.ev.key && lt.ev.ascii != ';')
		w = w * 10 + lt.ev.ascii - '0';
	while (get_live(&lt) && !lt.ev.key && lt.ev.ascii != ';')
		h = h * 10 + lt.ev.ascii - '0';
	
	char cmd[50];
	int ccmd = sprintf(cmd, "\033l\033c%u;%u;", w/2, h/2);
	write(STDOUT_FILENO, cmd, ccmd);
	write(STDOUT_FILENO, "test", 4);

	ccmd = sprintf(cmd, "\033Ca%u;%u;", w/2, h/2 + 1);
	write(STDOUT_FILENO, cmd, ccmd);

	while (get_live(&lt) && lt.ev.key != KEY_BACKSPACE);
	return 0;
}
