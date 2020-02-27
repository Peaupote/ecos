#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include <stdint.h>

typedef uint8_t keycode;

extern keycode keyboard_input_keycode;

extern unsigned char keyboard_azerty_layout[128][4];
extern unsigned char keyboard_qwerty_layout[128][4];
extern uint8_t use_azerty;

extern uint8_t keyboard_state[128 / 8];

extern keycode key_enter;
extern keycode key_backspace;
extern keycode key_lmaj;
extern keycode key_rmaj;

static inline unsigned char keycode_to_ascii(keycode c) {
	uint8_t i = ( keyboard_state[key_lmaj>>3]>>(key_lmaj&0x7)
			    | keyboard_state[key_rmaj>>3]>>(key_rmaj&0x7)
				) & 1;
	return (use_azerty ? keyboard_azerty_layout : keyboard_qwerty_layout)
				[c & 0x7f][i];
}
static inline unsigned char keycode_to_printable(keycode c) {
	char ca = keycode_to_ascii(c);
	return (ca < 0x20 || ca > 0x7e) ? 0 : ca;
}
static inline void keyboard_update_state(keycode k) {
	uint8_t m = 1 << (k & 0x7);
	uint8_t i = ((k>>3) & 0xf);
	if (k & 0x80) 
		keyboard_state[i] &= ~m;
	else
		keyboard_state[i] |=  m;
}

#endif
