#ifndef _HD_DISPLAY_H
#define _HD_DISPLAY_H

#define DISPLAY_FILE "/proc/display"

#include <stdint.h>

typedef uint32_t rgbcolor_t;

enum display_info_ty {
	display_type_err,
	display_type_text,
	display_type_graph
};

struct display_info {
	enum display_info_ty type;
	unsigned width;
	unsigned height;
};

enum display_rq_ty {
	display_rq_acq,
	display_rq_nacq,
	display_rq_clear,
	display_rq_write
};

struct display_rq {
	enum display_rq_ty type;
	unsigned top, left, bot, right;
	unsigned bpp, pitch; // en octets
	rgbcolor_t* data;
};

#endif
