#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include <headers/keyboard.h>

#include <stdbool.h>

typedef uint8_t scancode_byte;

extern unsigned char keyboard_azerty_layout[128][4];
extern unsigned char keyboard_qwerty_layout[128][4];
extern uint8_t use_azerty;

extern uint8_t keyboard_state[256 / 8];

static inline bool key_state(keycode k) {
    return (keyboard_state[k>>3]>>(k & 0x7)) & 1;
}

// Touches lshift / rshift, ne tient pas compte de verr. maj.
static inline bool key_state_shift() {
    return key_state(KEY_LSHIFT) || key_state(KEY_RSHIFT);
}
// TODO: verr. maj.
static inline bool key_state_maj() {
    return key_state_shift();
}
static inline bool key_state_altgr() {
    return key_state(KEY_ALTGR);
}
static inline bool key_state_ctrl() {
	return key_state(KEY_LCTRL) || key_state(KEY_RCTRL);
}

static inline uint8_t key_state_quad() {
    return (key_state_maj() ? 1 : 0)
        | (key_state_altgr() ? 2 : 0);
}
static inline unsigned char keycode_to_ascii(keycode c) {
    if (c & 0x80) return 0;
    return (use_azerty ? keyboard_azerty_layout : keyboard_qwerty_layout)
                [c & 0x7f][key_state_quad()];
}

key_event keyboard_update_state(scancode_byte);

#endif
