#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "../../src/kernel/header/keyboard.h"

#include <stdint.h>

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

/* --État des touches--
 * bit0 à 1 si appuyée, 0 sinon */

static inline uint8_t key_state(keycode k) {
    return keyboard_state[k>>3]>>(k & 0x7);
}

// Touches lshift / rshift, ne tiend pas compte de verr. maj.
static inline uint8_t key_state_shift() {
    return key_state(KEY_LSHIFT) | key_state(KEY_RSHIFT);
}
// TODO: verr. maj.
static inline uint8_t key_state_maj() {
    return key_state_shift();
}
static inline uint8_t key_state_altgr() {
    return key_state(KEY_ALTGR);
}


static inline uint8_t key_state_quad() {
    return (key_state_maj() & 1)
        | ((key_state_altgr() & 1) << 1);
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
