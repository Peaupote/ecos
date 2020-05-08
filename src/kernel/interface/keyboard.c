#include <kernel/keyboard.h>

#include <kernel/kutil.h>

uint8_t use_azerty = 1;

uint8_t keyboard_state[256 / 8] = {0};

static inline void set_key_state(uint8_t kid, bool pressed) {
    uint8_t m = 1   << (kid & 0x7);
    uint8_t i = kid >> 3;
    if (pressed)
        keyboard_state[i] |=  m;
    else
        keyboard_state[i] &= ~m;
}

uint8_t seq_state = 0;

key_event keyboard_update_state(scancode_byte s) {
    key_event ev = { 0 };
    switch(seq_state){
        case 0:
            if (s == 0xE0){
                seq_state = 1;
                return ev;
            }
            ev.key = s & 0x7F;
			break;
        case 1:
            ev.key = s | 0x80;
            seq_state = 0;
			break;
        default: never_reached
    }
	ev.flags = (s & 0x80) ? 0 : KEY_FLAG_PRESSED;
	if (key_state(ev.key))  ev.flags |= KEY_FLAG_PREV;
	set_key_state(ev.key, !(s & 0x80));
	
	if (key_state_shift())  ev.flags |= KEY_FLAG_SHIFT;
	if (key_state_ctrl())   ev.flags |= KEY_FLAG_CTRL;
	if (key_state(KEY_ALT)) ev.flags |= KEY_FLAG_ALT;
	uint8_t qd = key_state_quad();
	ev.flags |= qd << KEY_FLAG_QUAD_SHIFT;
    ev.ascii = (ev.key & 0x80) ? 0
				: (use_azerty 
						? keyboard_azerty_layout
						: keyboard_qwerty_layout)
				   [ev.key & 0x7f][qd];
	return ev;
}
