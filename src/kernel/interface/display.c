#include <kernel/display.h>

#include <kernel/memory/kmem.h>
#include <kernel/kutil.h>
#include <kernel/tty.h>
#include <kernel/sys.h>

#include <util/vga.h>

struct display_info display_info;
bool     display_graph;

static unsigned hgt, wdt;
static int      vbe_mode = ~0;
static uint8_t  type;
static unsigned bpp, pitch;
static uint8_t  cshift[3];//RGB
static bool     is_bgr;
static uint8_t  cmask[3];
static union {
	uint8_t*  p;
	uint16_t* c;
} pixbuf;

static size_t   char_width  = 9;
static size_t   char_height = 18;
extern uint8_t  font_data[];

#ifdef __is_debug
static long unsigned dchar_count = 0;
#endif

static inline bool is_egat() {
	return type == MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT;
}

void display_clear() {
	if (is_egat()) {
		for (size_t y = 0; y < hgt; ++y)
			for (size_t x = 0; x < wdt; ++x)
				pixbuf.c[y*pitch + x*bpp] = vga_entry(' ',
						vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_BLACK));
	} else {
		for (size_t y = 0; y < hgt; ++y)
			for (size_t x = 0; x < wdt; ++x)
				for (uint8_t i = 0; i < 3; ++i)
					pixbuf.p[y*pitch + x*bpp + cshift[i]] = 0;
	}
}

static void* map_pixbuf(phy_addr buf_begin, phy_addr buf_end) {

	phy_addr phy_it = buf_begin & paging_lvl_trunc(pgg_pd);
	uint_ptr   v_it = paging_set_lvl(pgg_pml4, PML4_KERNEL_VIRT_ADDR)
					| paging_set_lvl(pgg_pdpt, KERNEL_PDPT_PBUF);
	void* rt = (void*)(v_it + (buf_begin & ~paging_lvl_trunc(pgg_pd)));

	for (; phy_it < buf_end; phy_it += paging_add_lvl(pgg_pd, 1),
						  	 v_it   += paging_add_lvl(pgg_pd, 1))
		*kmem_acc_pts_entry(v_it, pgg_pd, PAGING_FLAG_W)
			= phy_it
			| PAGING_FLAG_P | PAGING_FLAG_W | PAGING_FLAG_S;
	
	return rt;
}

bool display_init(const multiboot_info_t* mbi) {
	unsigned flags = mbi->flags;

	if (flags & MULTIBOOT_INFO_VBE_INFO)
		vbe_mode = mbi->vbe_mode;

	if (! (flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO))
		goto unsupported_display;

	phy_addr buf = mbi->framebuffer_addr;
	type         = mbi->framebuffer_type;
	hgt          = mbi->framebuffer_height,
	wdt          = mbi->framebuffer_width;

	uint8_t bppi = mbi->framebuffer_bpp;//bits per pixel
	if (bppi % 8) goto unsupported_display;
	bpp    = bppi / 8;

	pitch = mbi->framebuffer_pitch;
	
	if (type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {

		uint8_t cshifti[3] = {
			mbi->framebuffer_red_field_position,
			mbi->framebuffer_green_field_position,
			mbi->framebuffer_blue_field_position
		};
		for (uint8_t i = 0; i < 3; ++i) {
			if (cshifti[i] % 8) goto unsupported_display;
			cshift[i] = cshifti[i] / 8;
		}

		cmask[0] = mbi->framebuffer_red_mask_size;
		cmask[1] = mbi->framebuffer_blue_mask_size;
		cmask[2] = mbi->framebuffer_green_mask_size;

		is_bgr = cshift[0] == 2
			  && cshift[1] == 1
			  && cshift[2] == 0;

		pixbuf.p = (uint8_t*) map_pixbuf(buf, buf + hgt * pitch);
		
		display_info.type = display_type_graph;

	} else if (type == MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT) {
        pixbuf.c = (uint16_t*) map_pixbuf(buf, buf + hgt * pitch);
		// en passe les variables en nombre de caractères
		pitch /= 2;
		bpp    = 1;
		
		display_info.type = display_type_text;
	}

	else goto unsupported_display;
	
	display_info.width  = wdt;
	display_info.height = hgt;
	
	display_clear();
	return true;

unsupported_display:
	display_info.type   = display_type_err;
	display_info.width  = 0;
	display_info.height = 0;
	return false;
}

void display_putpixel(unsigned x, unsigned y, rgbcolor_t c) {
	pixbuf.p[y * pitch + x * bpp + cshift[0]] = (c >> 16) & 0xff;
	pixbuf.p[y * pitch + x * bpp + cshift[1]] = (c >> 8)  & 0xff;
	pixbuf.p[y * pitch + x * bpp + cshift[2]] = c & 0xff;
}
void display_draw_rect(unsigned left, unsigned top, 
					   unsigned w, unsigned h, rgbcolor_t c) {
	for (unsigned y = top; y < top + h; ++y)
		for (unsigned x = left; x < left + w; ++x) {
			pixbuf.p[y * pitch + x * bpp + cshift[0]] = (c >> 16) & 0xff;
			pixbuf.p[y * pitch + x * bpp + cshift[1]] = (c >> 8)  & 0xff;
			pixbuf.p[y * pitch + x * bpp + cshift[2]] = c & 0xff;
		}
}

size_t display_char_width() {
	return is_egat() ? wdt  : wdt  / char_width;
}
size_t display_char_height() {
	return is_egat() ? hgt : hgt / char_height;
}
static inline uint8_t interpole(uint8_t t, uint8_t v0, uint8_t v1) {
	return (t * v1 + (0xff - t) * v0) / 0xff;
}

void display_char_at(size_t x, size_t y, char c, rgbcolor_t fg, rgbcolor_t bg) {
	unsigned left = x * char_width,
	          top = y * char_height;
	if (c <= 0x20 || c >= 0x7f)
		display_draw_rect(left, top, char_width, char_height, bg);
	else {
		uint8_t* char_data = font_data + (c - 0x21) * char_width * char_height;
		for (unsigned ry = 0; ry < char_height; ++ry)
			for (unsigned rx = 0; rx < char_width; ++rx) {
				uint16_t v = char_data[ry * char_width + rx];
				uint8_t* pix = pixbuf.p + (top + ry)*pitch + (left + rx)*bpp;
				pix[cshift[0]] = interpole(v, (bg >> 16) & 0xff, (fg >> 16) & 0xff);
				pix[cshift[1]] = interpole(v, (bg >> 8) & 0xff, (fg >> 8) & 0xff);
				pix[cshift[2]] = interpole(v, bg & 0xff, fg & 0xff);
			}
	}
}

static rgbcolor_t colors[16] = {
	0x000000, // black
	0x0000a8, // blue
	0x00a800, // green
	0x00a8a8, // cyan
	0xa80000, // red
	0xa800a8, // magenta
	0xa85700, // brown
	0xa8a8a8, // light grey
	0x333333, // dark grey
	0x5757ff, // light blue
	0x57ff57, // light green
	0x57ffff, // light cyan
	0xff5757, // light red
	0xff57ff, // light magenta
	0xffb10f, // light brown
	0xffffff  // white
};

void display_echar_at(size_t x, size_t y, uint16_t e) {
#ifdef __is_debug
	++dchar_count;
#endif
	if (is_egat())
		pixbuf.c[y * pitch + x] = e;
	else {
		char c = e & 0xff;
		uint8_t fg = (e >> 8) & 0xf;
		uint8_t bg = (e >> 12) & 0xf;
		display_char_at(x,y, c, colors[fg], colors[bg]);
	}
}

void display_cursor_at(size_t x, size_t y, uint16_t e) {
	if (is_egat())
		pixbuf.c[y * pitch + x] = vga_entry_inv(e);
	else {
		unsigned left = x * char_width,
				  top = y * char_height;
		display_draw_rect(left, top + 2, 1, char_height - 4, 0xeeeeee);
	}
}

void display_print_info() {
	kprintf("ty:%d w:%d h:%d bpp=%d pitch=%d bgr=%d\n"
			"dc:%lu cw:%d ch:%d\n"
			"Ro:%d Vo:%d Go:%d\n"
			"Rm:%d Vm:%d Gm:%d\n"
			,
			(int)type, (int)wdt, (int)hgt,
			(int)bpp, (int)pitch, (int)(is_bgr ? 1 : 0),
#ifdef __is_debug
			dchar_count,
#else
			0,
#endif
			(int)char_width, (int)char_height,
			(int)cshift[0], (int)cshift[1], (int)cshift[2],
			(int)cmask[0], (int)cmask[1], (int)cmask[2]);
}


static bool modeb = false;

bool display_set_graph(bool g) {
	if (g && is_egat()) return false;
	modeb = g;
	if (display_graph != modeb) {
		if (modeb) {
			if (tty_display_graph_rq(true)) {
				display_clear();
				display_graph = true;
			}
		} else {
			display_graph = false;
			display_clear();
			tty_display_graph_rq(false);
		}
	}
	return true;
}
void display_set_debug(bool d) {
	if (d) {
		if (display_graph) {
			display_clear();
			display_graph = false;
			tty_display_graph_rq(false);
		}
	} else if (modeb) {
		if (tty_display_graph_rq(true)) {
			display_clear();
			display_graph = true;
		}
	}
}

static bool display_draw_rq(struct display_rq* rq) {
	unsigned top   = rq->top,
			 bot   = rq->bot,
			 left  = rq->left,
			 right = rq->right;
	unsigned sbpp   = rq->bpp,
			 spitch = rq->pitch;
	if (!display_graph
	|| top  >= bot   || bot   > hgt
	|| left >= right || right > wdt
	|| spitch > 0x100000 || sbpp > 0x100000
	|| !check_argR(rq->data, (bot - top - 1) * (uint64_t)spitch + 
							(right - left) * (uint64_t)sbpp))
		return false;

	uint8_t *src0 = (uint8_t*)(rq->data),
		    *dst0 = pixbuf.p + top*pitch + left*bpp;

	if (is_bgr) {
		uint8_t *dst0_lim = dst0 + pitch * (bot - top);
		uint64_t x_len = bpp * (right - left);
		for (; dst0 < dst0_lim; dst0 += pitch) {
			uint8_t* src = src0;
			uint8_t* dst = dst0;
			uint8_t* dst_lim = dst + x_len;
			for (; dst < dst_lim; dst += bpp) {
				*(uint16_t*)dst = *(uint16_t*)src;
				dst[2] = src[2];
				src += sbpp;
			}
			src0 += spitch;
		}
	} else {
		for (unsigned y = top; y < bot; ++y) {
			uint8_t* src = src0;
			uint8_t* dst = dst0;
			for (unsigned x = left; x < right; ++x) {
				dst[cshift[0]] = src[2];
				dst[cshift[1]] = src[1];
				dst[cshift[2]] = src[0];
				src += sbpp;
				dst += bpp;
			}
			src0 += spitch;
			dst0 += pitch;
		}
	}
	
	return true;
}

int display_handle_rq(struct display_rq* rq) {
	switch (rq->type) {
		case display_rq_acq:
			display_set_graph(true);
			return sizeof(struct display_rq);
		case display_rq_nacq:
			display_set_graph(false);
			return sizeof(struct display_rq);
		case display_rq_clear:
			display_clear();
			return sizeof(struct display_rq);
		case display_rq_write:
			return display_draw_rq(rq) ? sizeof(struct display_rq) : 0;
	}
	return 0;
}
