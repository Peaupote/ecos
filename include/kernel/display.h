#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <util/multiboot.h>
#include <stdint.h>
#include <stddef.h>

typedef uint64_t rgbcolor_t;

unsigned display_height, display_width;

void display_init(const multiboot_info_t*);
void display_clear();

size_t  display_char_width();
size_t  display_char_height();
void    display_char_at(size_t x, size_t y, char c,
				rgbcolor_t fg, rgbcolor_t bg);
void    display_echar_at(size_t x, size_t y, uint16_t e);

#endif
