#ifndef _TTY_H
#define _TTY_H

#include <stddef.h>
#include <stdint.h>

#include "keyboard.h"

void   tty_init(void);
void   tty_input(scancode_byte, key_event);

//L'affichage du prompt doit être mis à jour (tty_update_prompt_pos)
void   tty_afficher_buffer_range(size_t idx_begin, size_t idx_end);
void   tty_afficher_buffer_all();

size_t tty_new_prompt(void);
void   tty_afficher_prompt(void);
size_t tty_update_prompt_pos(void);
size_t tty_prompt_to_buffer(size_t in_begin, size_t in_len);

size_t tty_buffer_cur_idx ();
size_t tty_buffer_next_idx();

//indice sans shift dans le buffer
//retourne le shift_array effectué
size_t tty_new_buffer_line(size_t* index);

void   tty_force_new_line(void);
size_t tty_writestring(const char* str);
size_t tty_writestringl(const char* s, size_t len);
void   tty_writer(void* shift, const char* str);

typedef struct {
	size_t idx_bg;
	size_t shift;
} tty_seq_t;
static inline void tty_seq_init(tty_seq_t* s) {
	s->idx_bg = tty_buffer_cur_idx();
	s->shift  = 0;
}
static inline void tty_seq_commit(tty_seq_t* s) {
	s->shift += tty_update_prompt_pos();
    if (s->shift) tty_afficher_buffer_all();
    else tty_afficher_buffer_range(s->idx_bg, tty_buffer_next_idx());
}
void tty_seq_write(void* seq, const char* s, size_t len);

#endif