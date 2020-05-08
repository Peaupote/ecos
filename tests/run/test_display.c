#include <headers/display.h>
#include <headers/keyboard.h>

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

const char type_char[3] = {'E', 'T', 'G'};

int main() {
	int fd = open(DISPLAY_FILE, READ|WRITE);
	if (fd < 0) return 1;

	struct display_info di;
	if (read(fd, &di, sizeof(di)) != sizeof(di))
		return 2;
	
	printf("%c %d %d\n", type_char[di.type], di.width, di.height);

	char in;
	read(STDIN_FILENO, &in, 1);

	if (in != '\n') return 0;

	printf("\033l"); fflush(stdout);

	struct display_rq rq;
	rq.type = display_rq_acq;
	write(fd, &rq, sizeof(rq));

	rq.type  = display_rq_write;
	rq.top   = 0;
	rq.bot   = 50;
	rq.left  = 0;
	rq.right = 50;
	rq.bpp   = 4;
	rq.pitch = 4 * 50;
	int buf[50][50];
	for (int i = 0; i < 50; ++i)
		for (int j = 0; j < 50; ++j)
			buf[i][j] = ((i * 5) << 0x10) + ((j*5) << 0x08);
	rq.data = buf;
	printf("wc = %d\n", write(fd, &rq, sizeof(rq)));

	while (1) {
		key_event ev;
		while(read(STDIN_FILENO, &ev, sizeof(ev)) != sizeof(ev));
		if (ev.key == KEY_RIGHT_ARROW) {
			rq.left  += 25;
			rq.right += 25;
			if (rq.right > di.width) {
				rq.right = di.width;
				rq.left = rq.right - 50;
				for (int i = 0; i < 50; ++i)
					for (int j = 0; j < 50; ++j)
						buf[i][j] = (i * 5) + ((j*5) << 0x10);
			}
			write(fd, &rq, sizeof(rq));
		}
	}
}
