#ifndef _TTY_H
#define _TTY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <kernel/keyboard.h>

#include <headers/proc.h>

#include <libc/stdio_strfuns.h>

enum tty_mode {
    ttym_def,
	ttym_prompt,
	ttym_live,
    ttym_debug,
    ttym_panic,
};

typedef struct {
	size_t x, y;
} tty_pos_t;


extern size_t  tty_width, tty_height;
extern size_t  tty_sb_height;
extern bool    tty_do_kprint;
extern uint8_t tty_def_color;

void   tty_init(enum tty_mode);
void   tty_set_mode(enum tty_mode);
void   tty_set_owner(pid_t p);
bool   tty_input(scancode_byte scb, key_event ev);
void   tty_on_pit();

// L'affichage du prompt doit être mis à jour avant
// (tty_update_prompt)
void   tty_afficher_buffer_range(
		    size_t idx_bg, size_t x_bg,
		    size_t idx_ed, size_t x_ed);
void   tty_afficher_buffer_all();

size_t tty_new_prompt(void);
void   tty_afficher_prompt(bool fill);
size_t tty_update_prompt(void);
size_t tty_prompt_to_buffer(size_t in_len);

void   tty_afficher_all();

size_t tty_buffer_next_idx();

// indice sans shift dans le buffer
// retourne le shift_array effectué
size_t tty_new_buffer_line_idx(size_t* index);

void   tty_force_new_line(void);
size_t tty_writestringl(const char* s, size_t len, uint8_t color);
void   tty_writer(void* shift, const char* str);


typedef struct {
    size_t idx_bg;
	size_t x_bg;
    size_t shift;
} tty_seq_t;

void   tty_seq_init(tty_seq_t* s);
void   tty_seq_write(void* seq, const char* s, size_t len);
void   tty_seq_commit(tty_seq_t* s);

static inline int tty_seq_printf(tty_seq_t* sq, const char* fmt, ...) {
    va_list ps;
    int count;
    va_start(ps, fmt);
    count = fpprintf(&tty_seq_write, sq, fmt, ps);
    va_end(ps);
    return count;
}

// write on ttyi
size_t tty_writei(uint8_t num, const char* str, size_t len);

void   tty_built_in_exec(tty_seq_t* sq, char* cmd);

#endif
