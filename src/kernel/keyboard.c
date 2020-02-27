#include "keyboard.h"

uint8_t use_azerty = 1;

uint8_t keyboard_input_keycode = 0;

keycode key_enter = 0x1C;
keycode key_backspace = 0x0E;
keycode key_lmaj = 0x2A;
keycode key_rmaj = 0x36;

uint8_t keyboard_state[128 / 8] = {0};
