#ifndef _GRAPH_H
#define _GRAPH_H

#include <headers/display.h>

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	unsigned w, h;
	unsigned ml, mt;
	rgbcolor_t c[];
} graph_buf_t;

int graph_open_display();

bool graph_read_infos(int fd, struct display_info* di);

bool graph_acq_display(int fd);

graph_buf_t* graph_make_buf(unsigned margin_left, unsigned margin_top,
		unsigned w, unsigned h);

bool graph_display_rect(int fd, const graph_buf_t* buf,
		unsigned left,  unsigned top,
		unsigned right, unsigned bot);

static inline bool graph_display(int fd, const graph_buf_t* buf) {
	return graph_display_rect(fd, buf, 0, 0, buf->w, buf->h);
}

// Les coordonnées doivent êtres dans les limites ([0, w[, [0, h[)
void graph_draw_line(graph_buf_t* buf,
		unsigned ax, unsigned ay, unsigned bx, unsigned by,
		rgbcolor_t color);

#endif
