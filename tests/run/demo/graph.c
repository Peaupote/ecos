#include "graph.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <util/misc.h>

#define DEPTH_MASK  0xff000000
#define DEPTH_SHIFT 24

int graph_open_display() {
	return open(DISPLAY_FILE, READ|WRITE);
}

bool graph_read_infos(int fd, struct display_info* di) {
	return read(fd, di, sizeof(*di)) == sizeof(*di)
		&& di->type == display_type_graph;
}
bool graph_acq_display(int fd) {
	struct display_rq rq;
	rq.type = display_rq_acq;
	return write(fd, &rq, sizeof(rq)) == sizeof(rq);
}
bool graph_nacq_display(int fd) {
	struct display_rq rq;
	rq.type = display_rq_nacq;
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
	return rt;
}

void graph_clear(graph_buf_t* buf) {
	for (unsigned i = 0; i < buf->h * buf->w; ++i) buf->c[i] = 0;//black
}

void graph_clear3(graph_buf_t* buf) {
	for (unsigned i = 0; i < buf->h * buf->w; ++i)
		buf->c[i] = DEPTH_MASK;
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

static inline void graph_draw_trap(graph_buf_t* buf,
		int      x0[2],  //x0[0] <= x0[1]
		int      dx0[2], int      dx1[2],
		unsigned dy0[2], unsigned dy1[2],
		int      py[2], // dx1 / dy1
		unsigned y_bg,   unsigned y_ed, //y_bg <= y_ed
		fxm_t depth_lin[2], fxm_t depth_ofs_bg,
		rgbcolor_t color) {

	int xinc[2] = {dx1[0] > 0 ? 1 : -1,
	               dx1[1] > 0 ? 1 : -1};

	unsigned dy[2] = {dy0[0], dy0[1]};
	int      x [2] = {x0 [0], x0 [1]};
	int     sdx[2] = {dx0[0] - x0[0], dx0[1] - x0[1]};

	unsigned     y = y_bg;
	fxm_t    depth = depth_ofs_bg;
	rgbcolor_t* ln = buf->c + y_bg * buf->w;
	/*fprintf(stderr, "draw trap %d - %d, %d(%d) - %d(%d)\n",
			y_bg, y_ed, x[0], py[0], x[1], py[1]);*/
	goto loop_enter;
	do {
		for (uint8_t k = 0; k < 2; ++k) {
			++dy[k];
			x[k] += py[k];
			if (((x[k] + sdx[k])* (int)dy1[k] < dx1[k] * (int)dy[k])
					^ (xinc[k] < 0))
				x[k]  += xinc[k];
		}
		ln    += buf->w;
		depth += depth_lin[1];
	loop_enter:;
		unsigned x_left = max_int(0, x[0]);
		rgbcolor_t *left  = ln + x_left,
			       *right = ln + min_int(buf->w - 1, x[1]);
		fxm_t depth1 = depth + x_left * depth_lin[0];
		for (rgbcolor_t* it = left; it <= right; ++it, depth1 += depth_lin[0])
			if (((*it) >> (DEPTH_SHIFT - FXM_DIGITS)) >= (uint32_t)depth1)
				*it = color | ((depth1 << 8) & DEPTH_MASK);
		++y;
	} while(y <= y_ed);
}

typedef struct {
	int x, y;
} gpos_t;

static inline void cmp_sparam(gpos_t pos[],
		int* dx1, unsigned* dy1, int* py) {
	*dx1 = pos[1].x - pos[0].x;
	*dy1 = 1 + pos[1].y - pos[0].y;
	*py  = (*dx1) / (int)(*dy1);
}

void graph_draw_tri(graph_buf_t* buf,
		int x[3], int y[3], fxm_t depth[3],
		rgbcolor_t color) {

	fxm_t delta_pos[4] = {
			fxm_of_int(x[1] - x[0]), fxm_of_int(y[1] - y[0]),
			fxm_of_int(x[2] - x[0]), fxm_of_int(y[2] - y[0])
		};
	fxm_t delta_depth[2] = {depth[1] - depth[0], depth[2] - depth[0]};
	/*fprintf(stderr, "\ndepths: %d, %d, %d\n",
			fxm_to_int(depth[0]),
			fxm_to_int(depth[1]),
			fxm_to_int(depth[2]));
	fprintf(stderr, "delta pos\n");
	fxm_mprint(stderr, delta_pos, 2, 2, 3);
	fprintf(stderr, "delta depth\n");
	fxm_mprint(stderr, delta_depth, 1, 2, 3);*/

	fxm_t depth_lin[2];
	if (!fxm_mslv2(delta_pos, delta_depth, depth_lin)) return;
	
	fxm_t depth_ofs = depth[0] - (depth_lin[0] * x[0] + depth_lin[1] * y[0]);
	/*fxm_t test[4];
	fxm_mmul(delta_pos, depth_lin, test, 2, 2, 1);
	fprintf(stderr, "depth lin\n");
	fxm_mprint(stderr, depth_lin, 1, 2, 3);
	fprintf(stderr, "test\n");
	fxm_mprint(stderr, test, 1, 2, 3);*/

	for (uint8_t i = 0; i < 3; ++i) {
		uint8_t j = i & 1;
		if (y[j] > y[j + 1]) {
			swap_int(y + j, y + j + 1);
			swap_int(x + j, x + j + 1);
		}
	}
	gpos_t sides  [2][2];// left/right - begin/end
	{
		sides[0][0].x = sides[1][0].x = x[0];
		sides[0][0].y = sides[1][0].y = y[0];
		uint8_t s1 = (x[2] - x[0]) * (y[1] - y[0]) 
			       > (x[1] - x[0]) * (y[2] - y[0])
				   ? 0 : 1;
		sides[s1][1].x = x[1];
		sides[s1][1].y = y[1];
		sides[1^s1][1].x = x[2];
		sides[1^s1][1].y = y[2];
	}
	int      dx1[2];
	unsigned dy1[2];
	int      py [2];
	int     ylim[2];
	for (uint8_t k = 0; k < 2; ++k) {
		cmp_sparam(sides[k], dx1 + k, dy1 + k, py + k);
		ylim[k] = sides[k][1].y;
	}
	int ys[2]   = {y[0], y[1]};
	
	while (true) {
		/*fprintf(stderr, "%d <-> %d | %d,%d <-> %d,%d | %d,%d <-> %d,%d\n",
					ys[0], ys[1],
					sides[0][0].x, sides[0][0].y,
					sides[0][1].x, sides[0][1].y,
					sides[1][0].x, sides[1][0].y,
					sides[1][1].x, sides[1][1].y
				);*/
		if (ys[0] >= (int)buf->h) return;
		if (ys[1] >= 0) {
			unsigned y0 = max_int(0, ys[0]);
			int      xv [2];
			int      dx0[2];
			unsigned dy0[2];
			for (uint8_t k = 0; k < 2; ++k) {
				dy0[k] = y0 - sides[k][0].y;
				dx0[k] = dx1[k] * ((int)dy0[k]) / (int)dy1[k];
				xv [k] = sides[k][0].x + dx0[k];
			}

			graph_draw_trap(buf, xv, dx0, dx1, dy0, dy1, py,
							y0, min_int(buf->h - 1, ys[1]),
							depth_lin, depth_ofs + depth_lin[1] * y0,
							color);
		}
		ys[0] = ys[1];
		for (uint8_t k = 0; k < 2; ++k) {
			if (sides[k][1].y <= ys[0]) {
				if (y[2] <= ys[0]) return;
				sides[k][0] = sides[k][1];
				sides[k][1].x = x[2];
				sides[k][1].y = y[2];
				cmp_sparam(sides[k], dx1 + k, dy1 + k, py + k);
				ylim[k] = y[2];
			}
		}
		ys[1] = min_int(ylim[0], ylim[1]);
	}
}
