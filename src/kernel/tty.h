#ifndef _TTY_H
#define _TTY_H

#include <stddef.h>
#include <stdint.h>

#include "keyboard.h"

void   tty_init(void);
void   tty_input(scancode_byte, key_event);

//L'affichage du prompt doit être mis à jour ensuite (tty_update_prompt_pos)
void   tty_afficher_buffer_range(size_t idx_begin, size_t idx_end);
void   tty_afficher_buffer_all();

size_t tty_new_prompt(void);
void   tty_afficher_prompt(void);
size_t tty_update_prompt_pos(void);
size_t tty_prompt_to_buffer(size_t in_begin, size_t in_len);

size_t tty_buffer_next_idx();

//indice sans shift dans le buffer
//retourne le shift_array effectué
size_t tty_new_buffer_line(size_t* index);

void   tty_force_new_line(void);
size_t tty_writestring(const char* str);
void   tty_writer(void* shift, const char* str);

#endif
