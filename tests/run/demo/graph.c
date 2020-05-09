#include "graph.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <util/misc.h>

int graph_open_display() {
	return open(DISPLAY_FILE, READ|WRITE);
}

bool graph_read_infos(int fd, struct display_info* di) {
	return read(fd, di, sizeof(*di)) == sizeof(*di);
}
bool graph_acq_display(int fd) {
	struct display_rq rq;
	rq.type = display_rq_acq;
	return write(fd, &rq, sizeof(rq)) == sizeof(rq);
}

graph_buf_t* graph_make_buf(unsigned margin_left, unsigned margin_top,
		unsigned w, unsigned h) {
	graph_buf_t* rt = malloc(offsetof(graph_buf_t, c) 
							+ h * w * sizeof(rgbcolor_t));
	rt->w = w;
	rt->h = h;
	rt->ml = margin_left;
	rt->mt = margin_top;
	for (unsigned i = 0; i < h * w; ++i) rt->c[i] = 0;//black
	return rt;
}

bool graph_display_rect(int fd, const graph_buf_t* buf,
		unsigned left,  unsigned top,
		unsigned right, unsigned bot) {
	struct display_rq rq = {
		.type = display_rq_write,
		.top  = buf->mt + top,  .bot   = buf->mt + bot,
		.left = buf->ml + left, .right = buf->ml + right,
		.bpp   = sizeof(rgbcolor_t),
		.pitch = buf->w * sizeof(rgbcolor_t),
		.data  = (void*)buf->c
	};
	return write(fd, &rq, sizeof(rq)) == sizeof(rq);
}

static inline int cmp_line_p(unsigned* d0, int* id1,
		unsigned a1, unsigned b1, int* inc1) {
	++*d0;
	*inc1 = b1 > a1 ? 1 : -1;
	*id1 = ((int)b1) + *inc1 - (int)a1;
	return (*id1) / (int)*d0;
}

void graph_draw_line(graph_buf_t* buf,
		unsigned ax, unsigned ay, unsigned bx, unsigned by,
		rgbcolor_t color) {
#ifdef __is_debug
	if (ax >= buf->w
	 ||	bx >= buf->w) {
		fprintf(stderr, "x out of bounds\n");
		return;
	}
	if (ay >= buf->h
	 ||	by >= buf->h) {
		fprintf(stderr, "y out of bounds\n");
		return;
	}
#endif

	unsigned dx = ax > bx ? ax - bx : bx - ax,
	         dy = ay > by ? ay - by : by - ay;
	if (dx >= dy) {
		if (ay > by) {
			swap_unsigned(&ax, &bx);
			swap_unsigned(&ay, &by);
		}
		int idx, incx;
		int par_y = cmp_line_p(&dy, &idx, ax, bx, &incx);

		rgbcolor_t* it = buf->c + ay * buf->w + ax;
		int x = 0;
		for (unsigned y = 0; y != dy; ) {
			rgbcolor_t* xlim = it + par_y;
			++y;
			x += par_y;
			if (((x + incx) * dy < y * idx) ^ (idx < 0)) {
				xlim += incx;
				x    += incx;
			}
			for (; it != xlim; it += incx) *it = color;
			it += buf->w;
		}
	} else {
		if (ax > bx) {
			swap_unsigned(&ax, &bx);
			swap_unsigned(&ay, &by);
		}
		int idy, incy;
		int par_x = cmp_line_p(&dx, &idy, ay, by, &incy);
		
		rgbcolor_t* it = buf->c + ay * buf->w + ax;
		int y = 0;
		ssize_t it_inc = incy * (ssize_t)buf->w;
		for (unsigned x = 0; x != dx; ) {
			rgbcolor_t* ylim = it + par_x * (ssize_t)buf->w;
			++x;
			y += par_x;
			if (((y + incy) * dx < x * idy) ^ (idy < 0)) {
				ylim += it_inc;
				y    += incy;
			}
			for (; it != ylim; it += it_inc) *it = color;
			++it;
		}
	}
}
