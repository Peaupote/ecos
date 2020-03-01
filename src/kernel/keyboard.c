#include "keyboard.h"

uint8_t use_azerty = 1;

uint8_t keyboard_state[256 / 8] = {0};

static inline void set_key_state(uint8_t kid, uint8_t clear) {
	uint8_t m = 1   << (kid & 0x7);
	uint8_t i = kid >> 3;
	if (clear) 
		keyboard_state[i] &= ~m;
	else
		keyboard_state[i] |=  m;
}

uint8_t seq_state = 0;

key_event keyboard_update_state(scancode_byte s) {
	key_event ev = {0, 0};
	switch(seq_state){
		case 0:
			if (s == 0xE0){
				seq_state = 1;
				return ev;
			}
			ev.key = s & 0x7F;
			ev.flags = s >> 7;
			set_key_state(ev.key, ev.flags);
			return ev;
		case 1:
			ev.key = s | 0x80;
			ev.flags = s >> 7;
			set_key_state(ev.key, ev.flags);
			seq_state = 0;
			return ev;
		default://Non atteignable
			seq_state = 0;
	}
	return ev;
}

