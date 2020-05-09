#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <headers/display.h>

#include <util/multiboot.h>

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

extern bool display_graph;
extern struct display_info display_info;

bool   display_init(const multiboot_info_t*);
void   display_clear();

size_t display_char_width();
size_t display_char_height();
void   display_char_at(size_t x, size_t y, char c,
				rgbcolor_t fg, rgbcolor_t bg);
void   display_echar_at(size_t x, size_t y, uint16_t e);
void   display_cursor_at(size_t x, size_t y);

bool   display_set_graph(bool);
void   display_set_debug(bool);

int   display_handle_rq(struct display_rq* rq);

void   display_print_info();

#endif
