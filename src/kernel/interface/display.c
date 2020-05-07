#include <kernel/display.h>
#include <kernel/memory/kmem.h>
#include <util/vga.h>

static int      vbe_mode = ~0;
static uint8_t  type;
static size_t   bpp, pitch;
static uint8_t  cshift[3];//RGB
static union {
	uint8_t*  p;
	uint16_t* c;
} pixbuf;

static size_t   char_width = 9;
static size_t   char_height = 18;

extern uint8_t  font_data[];

static inline bool is_egat() {
	return type == MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT;
}

void display_clear() {
	if (is_egat()) {
		for (size_t y = 0; y < display_height; ++y)
			for (size_t x = 0; x < display_width; ++x)
				pixbuf.c[y*pitch + x*bpp] = vga_entry(' ',
						vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_BLACK));
	} else {
		for (size_t y = 0; y < display_height; ++y)
			for (size_t x = 0; x < display_width; ++x)
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

void display_init(const multiboot_info_t* mbi) {
	unsigned flags = mbi->flags;

	if (flags & MULTIBOOT_INFO_VBE_INFO)
		vbe_mode = mbi->vbe_mode;

	if (! (flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO))
		goto unsupported_display;

	phy_addr buf   = mbi->framebuffer_addr;
	type           = mbi->framebuffer_type;
	display_height = mbi->framebuffer_height,
	display_width  = mbi->framebuffer_width;

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

		pixbuf.p = (uint8_t*) map_pixbuf(buf, buf + display_height * pitch);

	} else if (type == MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT) {
        pixbuf.c = (uint16_t*) map_pixbuf(buf, buf + display_height * pitch);
		// en passe les variables en nombre de caractères
		pitch /= 2;
		bpp    = 1;
	}

	else goto unsupported_display;
	
	display_clear();
	return;

unsupported_display:
	return;
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
	return is_egat() ? display_width  : display_width  / char_width;
}
size_t display_char_height() {
	return is_egat() ? display_height : display_height / char_height;
}
static inline uint8_t interpole(uint8_t t, uint8_t v0, uint8_t v1) {
	return (t * v1 + (0xff - t) * v0) / 0xff;
}

void display_char_at(size_t x, size_t y, char c, rgbcolor_t fg, rgbcolor_t bg) {
	unsigned left = x * char_width,
	          top = y * char_height;
	if (c <= 0x20 || c >= 0x7f)
		display_draw_rect(left, top, char_width, char_height, 0);
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
	0x0000ff, // blue
	0x00ff00, // green
	0x00ffff, // cyan
	0xff0000, // red
	0xff00ff, // magenta
	0x813c12, // brown
	0xcccccc, // light grey
	0x333333, // dark grey
	0x7777ff, // light blue
	0x72ff72, // light green
	0x55bebe, // light cyan
	0xff7777, // light red
	0xbe55be, // light magenta
	0xbf510d, // light brown
	0xffffff  // white
};

void display_echar_at(size_t x, size_t y, uint16_t e) {
	if (is_egat())
		pixbuf.c[y * pitch + x] = e;
	else {
		char c = e & 0xff;
		uint8_t fg = (e >> 8) & 0xf;
		uint8_t bg = (e >> 12) & 0xf;
		display_char_at(x,y, c, colors[fg], colors[bg]);
	}
}

void display_cursor_at(size_t x, size_t y) {
	if (is_egat()) return;
	unsigned left = x * char_width,
	          top = y * char_height;
	display_draw_rect(left, top + 2, 1, char_height - 4, 0xeeeeee);
}
