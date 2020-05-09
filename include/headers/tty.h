#ifndef _HD_TTY_H
#define _HD_TTY_H

#include <headers/keyboard.h>

#define TTY_LIVE_MAGIC 0xFF

typedef struct {
	uint8_t magic; //TTY_LIVE_MAGIC
	key_event ev;
} tty_live_t;

#endif
