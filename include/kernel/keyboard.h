#ifndef _KEYBOARD_H
#define _KEYBOARD_H


#define KEY_BACKSPACE	0x0E
#define KEY_ENTER		0x1C
#define KEY_LSHIFT		0x2A
#define KEY_RSHIFT		0x36
#define KEY_TAB         0x0F

#define KEY_KP_ENTER	0x9C
#define KEY_ALTGR		0xB8
#define KEY_UP_ARROW	0xC8
#define KEY_DOWN_ARROW  0xD0

#define KEYS_MASK		0x7F
#define KEYS_ENTER		0x1C


#include <stdint.h>
#include <stdbool.h>

typedef uint8_t scancode_byte;
typedef uint8_t keycode;
typedef struct {
    keycode key;
    uint8_t flags;
} key_event;

extern unsigned char keyboard_azerty_layout[128][4];
extern unsigned char keyboard_qwerty_layout[128][4];
extern uint8_t use_azerty;

extern uint8_t keyboard_state[256 / 8];

static inline bool key_state(keycode k) {
    return (keyboard_state[k>>3]>>(k & 0x7)) & 1;
}

// Touches lshift / rshift, ne tiend pas compte de verr. maj.
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


static inline uint8_t key_state_quad() {
    return (key_state_maj() ? 1 : 0)
        | (key_state_altgr() ? 2 : 0);
}
static inline unsigned char keycode_to_ascii(keycode c) {
    if (c & 0x80) return 0;
    return (use_azerty ? keyboard_azerty_layout : keyboard_qwerty_layout)
                [c & 0x7f][key_state_quad()];
}
static inline unsigned char keycode_to_printable(keycode c) {
    char ca = keycode_to_ascii(c);
    return (ca < 0x20 || ca > 0x7e) ? 0 : ca;
}

key_event keyboard_update_state(scancode_byte);

#endif
