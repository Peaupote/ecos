#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include <stdint.h>

uint8_t keyboard_input_keycode;
/*keycode -> keychar (QWERTY)*/
unsigned char keyboard_char_map[128];

#endif
