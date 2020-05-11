#ifndef _HD_KEYBOARD_H
#define _HD_KEYBOARD_H

#define KEY_ESCAPE	    0x01
#define KEY_BACKSPACE	0x0E
#define KEY_ENTER		0x1C
#define KEY_LSHIFT		0x2A
#define KEY_RSHIFT		0x36
#define KEY_TAB         0x0F
#define KEY_LCTRL       0x1D
#define KEY_RCTRL       0x9D
#define KEY_ALT         0x38

#define KEY_D           0x20
#define KEY_C           0x2E
#define KEY_Z           0x11

#define KEY_KP_ENTER	0x9C
#define KEY_ALTGR		0xB8
#define KEY_UP_ARROW	0xC8
#define KEY_DOWN_ARROW  0xD0
#define KEY_LEFT_ARROW	0xCB
#define KEY_RIGHT_ARROW 0xCD

#define KEYS_MASK		0x7F
#define KEYS_ENTER		0x1C

#define KEY_FLAG_PRESSED   0x01
#define KEY_FLAG_PREV      0x02
#define KEY_FLAG_SHIFT     0x04
#define KEY_FLAG_CTRL      0x08
#define KEY_FLAG_ALT       0x10
#define KEY_FLAG_MAJ       0x20
#define KEY_FLAG_ALTGR     0x40
#define KEY_FLAG_QUAD_SHIFT   5
#define KEY_FLAG_MODS (KEY_FLAG_SHIFT|KEY_FLAG_CTRL\
					  |KEY_FLAG_ALT|KEY_FLAG_ALTGR)


#ifndef ASM_FILE

#include <stdint.h>

typedef uint8_t keycode;

typedef struct {
    keycode key;
	char    ascii;
    uint8_t flags;
} key_event;

#endif
#endif
