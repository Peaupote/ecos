#include <kernel/tty.h>

#include <stddef.h>
#include <stdbool.h>

#include <util/vga.h>
#include <util/elf64.h>
#include <libc/string.h>
#include <util/misc.h>

#include <kernel/memory/kmem.h>
#include <kernel/proc.h>
#include <kernel/int.h>
#include <kernel/display.h>

#include <fs/proc.h>

#define SB_LENGTH (512 * 80)

#define IB_LENGTH 256
#define IM_LENGTH 256

//Screen Buffer
// tty_sb_height   |.....|bbb|ddd|......|
// ashift          |---->|
// display_shift         |-->|
// nb_lines              |------>|
static uint16_t sbuffer[SB_LENGTH];
static size_t   sb_ashift; //shift array
static size_t   sb_display_shift; // <= sb_nb_lines
// < sb_height pour éviter 0 = sb_height lors du passage au modulo
static size_t   sb_nb_lines;
static bool     sb_dtcd; // scroll détaché

//Input Buffer
static char     ibuffer[IB_LENGTH];
static size_t   ib_size;
static size_t   ib_printed;
static size_t   input_height;
static size_t   input_bottom_line;
static size_t   input_top_line;

static char     input_msg[IM_LENGTH];
static uint16_t input_msg_len;
static uint16_t input_msg_cs;
static uint8_t  input_lmargin;
static size_t   input_width;
static size_t   input_min_height;

static uint8_t  input_color, prompt_color, vprompt_color,
                buf_color,    err_color;
uint8_t tty_def_color;

static enum tty_mode tty_mode;
static enum tty_mode tty_bmode;

bool   tty_do_kprint = false;

size_t tty_width, tty_height, tty_sb_height;

static size_t cur_ln_x;

static pid_t tty_owner = PID_NONE;


static inline void putat(char c, uint8_t col, size_t x, size_t y) {
	display_echar_at(x, y, vga_entry(c, col));
}
static inline void puteat(uint16_t e, size_t x, size_t y) {
	display_echar_at(x, y, e);
}


static inline size_t sb_rel_im_bg() {
    return sb_display_shift;
}
static inline size_t sb_rel_im_ed() {
    return min_size_t(sb_nb_lines,
            (tty_height - input_height + sb_display_shift)%tty_sb_height);
}
size_t sb_new_in_height(size_t h) {
    if (sb_dtcd) return 0;
    size_t old_ds = sb_display_shift;
    size_t dsp_ln = tty_height - h;
    sb_display_shift = dsp_ln < sb_nb_lines
            ? sb_nb_lines - dsp_ln : 0;
    return sb_display_shift - old_ds;

}
static inline size_t sb_sc_ed() {
    return sb_nb_lines - sb_display_shift;
}

void tty_init(enum tty_mode m) {
	tty_width  = display_char_width();
	tty_height = display_char_height();
	tty_sb_height = SB_LENGTH / tty_width;

    tty_def_color = vga_entry_color (
                	VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    buf_color   = tty_def_color;
    err_color   = vga_entry_color (
                	VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    input_color = vga_entry_color (
                	VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    ib_printed   = ib_size = 0;
    input_height = 0;
    input_bottom_line = input_top_line = ~0;

    sb_ashift   = 0;
    sb_display_shift = 0;
    sb_nb_lines = 0;
    sb_dtcd     = false;

	tty_force_new_line();

	tty_bmode = m;
    tty_set_mode(m);
}

void tty_afficher_prompt_all();
void tty_set_mode(enum tty_mode m) {
    switch (m) {
	case ttym_prompt:
        prompt_color = vga_entry_color (
                    VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
        vprompt_color = vga_entry_color (
                    VGA_COLOR_BLUE, VGA_COLOR_BLACK);
		input_lmargin = 2;
        break;
    case ttym_def:
        prompt_color = vga_entry_color (
                    VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vprompt_color = vga_entry_color (
                    VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
		input_lmargin = 0;
		break;
    case ttym_panic:
        prompt_color = vga_entry_color (
                    VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vprompt_color = vga_entry_color (
                    VGA_COLOR_RED, VGA_COLOR_BLACK);
        goto set_defmsg;

    case ttym_debug:
        prompt_color = vga_entry_color (
                    VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vprompt_color = vga_entry_color (
                    VGA_COLOR_GREEN, VGA_COLOR_BLACK);
	set_defmsg:
		input_msg[0] = '>';
		input_msg_len = 1;
		input_lmargin = 2;
    }
	input_width = tty_width - input_lmargin;
	input_msg_cs = input_msg_len % tty_width;
	input_msg_cs = input_msg_cs > input_lmargin 
							? input_msg_cs - input_lmargin
							: 0;
	size_t input_msg_height = input_msg_len / tty_width;
	input_min_height = max_size_t(1, input_msg_height);
	input_msg_cs += input_msg_height * input_width;

    tty_mode = m;

    if (~input_top_line)//TODO height
		tty_afficher_prompt_all();
}

void tty_set_bmode(enum tty_mode m) {
	tty_bmode = m;
	if (tty_mode < ttym_debug)	
		tty_set_mode(m);
}

void tty_set_owner(pid_t p) {
    klogf(Log_info, "tty", "tty owner is %d", (int)p);
    tty_owner = p;
}


static inline size_t tty_input_to_buffer() {
	if (tty_mode == ttym_def)
		return tty_writestringl(ibuffer, ib_size, tty_def_color);
	else
		return tty_prompt_to_buffer(ib_size);
}

void tty_input(scancode_byte s, key_event ev) {
    tty_seq_t sq;
    tty_seq_init(&sq);

    if (tty_do_kprint) {
        char str[] = "s=__ c=__ f=__ p=__\n";
        int8_to_str_hexa(str + 2,  s);
        int8_to_str_hexa(str + 7,  ev.key);
        int8_to_str_hexa(str + 12, ev.flags);
        int8_to_str_hexa(str + 17, keycode_to_printable(ev.key));
        sq.shift += tty_writestringl(str, 20, tty_def_color);
        sq.shift += tty_update_prompt_pos();
    }
    if (ev.key && !(ev.flags & 0x1)) {//évènement appui
        if ((ev.key&KEYS_MASK) == KEYS_ENTER) {
			sq.shift += tty_input_to_buffer();
			tty_force_new_line();

            switch(tty_mode) {
            case ttym_def:
            case ttym_prompt:
                ibuffer[ib_size] = '\n';
				if(fs_proc_write_tty(ibuffer, ib_size + 1) != ib_size + 1)
					klogf(Log_error, "tty", "in_buffer saturated");
                break;
            default:
                ibuffer[ib_size] = '\0';
                sq.shift += tty_built_in_exec(ibuffer);
                break;
            }

            ib_size = ib_printed = 0;
            sq.shift += tty_new_prompt();
        } else if(ev.key == KEY_BACKSPACE) {//TODO
            if(ib_size > 0) {
                --ib_size;
                if(ib_printed > ib_size) ib_printed = ib_size;
                putat(' ', input_color,
					input_lmargin  + (ib_size + input_msg_cs) % input_width,
					input_top_line + (ib_size + input_msg_cs) / input_width);
            }
        } else if (ev.key == KEY_UP_ARROW) {
            if (sb_display_shift) {
                size_t mv = key_state_shift()
                          ? (tty_height - input_height) : 1;
                if (mv > sb_display_shift)
                    sb_display_shift = 0;
                else
                    sb_display_shift -= mv;
                sb_dtcd = true;
                tty_afficher_buffer_all();
            }
        } else if (ev.key == KEY_DOWN_ARROW) {
            if (sb_sc_ed() > tty_height - input_height) {
                size_t mv = key_state_shift()
                          ? (tty_height - input_height) : 1;
                mina_size_t(&mv,
                        sb_sc_ed() - (tty_height - input_height));
                sb_display_shift += mv;
                if (sb_sc_ed() == tty_height - input_height)
                    sb_dtcd = false;
                tty_afficher_buffer_all();
            }
        } else if (key_state_ctrl()) {
            switch (ev.key) {
                case KEY_TAB:
                switch (tty_mode) {
                    case ttym_def:    tty_set_mode(ttym_debug); break;
                    case ttym_prompt: tty_set_mode(ttym_debug); break;
                    case ttym_debug:  tty_set_mode(tty_bmode);   break;
                    case ttym_panic:  break;
                }
                break;
                case KEY_C:
                    if (~tty_owner && proc_alive(state.st_proc + tty_owner)) {
                		sq.shift += tty_writestringl("^C", 2, tty_def_color);
                        send_sig_to_proc(tty_owner, SIGINT - 1);
					}
                break;
				case KEY_D:
					if (ib_size) {
						sq.shift += tty_input_to_buffer();
						if(fs_proc_write_tty(ibuffer, ib_size) != ib_size)
							klogf(Log_error, "tty", "in_buffer saturated");
						ib_size = ib_printed = 0;
						sq.shift += tty_new_prompt();
					} else {
                		sq.shift += tty_writestringl("^D", 2, tty_def_color);
						fs_proc_send0_tty();
					}
				break;
            }
        } else {
            char kchar = keycode_to_printable(ev.key);
            if (kchar && ib_size < IB_LENGTH - 1) {
                ibuffer[ib_size++] = kchar;
                tty_afficher_prompt();
            }
        }
    }
    tty_seq_commit(&sq);
}

void tty_afficher_buffer_range(size_t idx_begin, size_t idx_end) {
    size_t r_b = idx_begin > sb_ashift 
			? (idx_begin - sb_ashift)%tty_sb_height
			: 0;
    size_t r_e = (idx_end - sb_ashift)%tty_sb_height;
    maxa_size_t(&r_b, sb_rel_im_bg());
    mina_size_t(&r_e, sb_rel_im_ed());

    for (size_t it = r_b; it < r_e; ++it) {
		uint16_t* line = sbuffer + ((sb_ashift + it)%tty_sb_height)*tty_width;
        for (size_t x = 0; x < tty_width; ++x)
            puteat(line[x], x, it - sb_display_shift);
	}
}

void tty_afficher_buffer_all() {
    size_t r_b = sb_rel_im_bg();
    size_t r_e = sb_rel_im_ed();

    for (size_t it = r_b; it < r_e; ++it) {
		uint16_t* line = sbuffer + ((sb_ashift + it)%tty_sb_height)*tty_width;
        for (size_t x = 0; x < tty_width; ++x)
            puteat(line[x], x, it - sb_display_shift);
	}
}

// --Prompt--

static tty_pos_t afficher_string(tty_pos_t pos, uint8_t lmrg, 
		size_t len, const char* str, uint8_t clr) {

	for (; pos.x < lmrg; ++pos.x)
		putat(' ', clr, pos.x, pos.y);
	for (size_t i = 0; i < len; ++pos.x, ++i) {
        if (pos.x >= tty_width) {
            ++pos.y;
			for (pos.x = 0; pos.x < lmrg; ++pos.x)
				putat(' ', clr, pos.x, pos.y);
        }
        putat(str[i], clr, pos.x, pos.y);
    }
	return pos;
}

static tty_pos_t afficher_input_msg() {
	tty_pos_t pos = {.x = 0, .y = input_top_line};
	return afficher_string(pos, 0, input_msg_len, input_msg, prompt_color);
}

size_t tty_new_prompt() {
    size_t rt = 0;
    input_height = 1;
    if (sb_sc_ed() >= tty_height) {
        input_top_line = tty_height - 1;
        rt = sb_new_in_height(1);
    } else
        input_top_line = sb_sc_ed();

    input_bottom_line = input_top_line;

	tty_pos_t pos = afficher_input_msg();
    for (; pos.x < tty_width; ++pos.x)
        putat(' ', input_color, pos.x, pos.y);

    return rt;
}

static inline size_t get_input_height () {
    size_t new_input_height = 
		(ib_size + input_msg_cs + input_width-1) / input_width;
    return max_size_t(input_min_height, new_input_height);
}

static void tty_afficher_prompt_do(bool fill) {
    tty_pos_t pos = {
            .x = input_lmargin + (ib_printed + input_msg_cs) % input_width,
            .y = input_top_line + (ib_printed + input_msg_cs) / input_width
        };

	pos = afficher_string(pos, input_lmargin, ib_size - ib_printed,
							ibuffer + ib_printed, input_color);
    if(fill)
        for(; pos.x < tty_width; ++pos.x)
            putat(' ', input_color, pos.x, pos.y);

    ib_printed = ib_size;
}

void tty_afficher_prompt_all() {
	afficher_input_msg();
    ib_printed = 0;
    tty_afficher_prompt_do(true);
}

void tty_afficher_prompt() {
    size_t new_input_height = get_input_height();
    size_t bt_line_0;

    if (new_input_height != input_height) {
        input_height = new_input_height;
        bt_line_0 = input_top_line + input_height - 1;

        if (bt_line_0 >= tty_height) {
            input_bottom_line = tty_height - 1;
            input_top_line = input_bottom_line - input_height + 1;
            sb_new_in_height(input_height);
            tty_afficher_buffer_all(); //scroll
            tty_afficher_prompt_all();
            return;
        }

        input_bottom_line = bt_line_0;
        tty_afficher_prompt_do(true);
        return;
    }
    tty_afficher_prompt_do(0);
}

size_t tty_update_prompt_pos() {//TODO
    if (~input_top_line) {
        size_t n_top;
        size_t rt = 0;
        if (sb_sc_ed() + input_height > tty_height) {
            n_top   = tty_height - input_height;
            rt      = sb_new_in_height(input_height);
        } else
            n_top = sb_sc_ed();

        if (n_top != input_top_line) {
            input_top_line    = n_top;
            input_bottom_line = input_top_line + input_height - 1;
            tty_afficher_prompt_all();
        }
        return rt;
    }
    return 0;//Pas de prompt
}

static size_t new_buffer_line(uint16_t** line) {
	size_t idx;
	size_t rt = tty_new_buffer_line_idx(&idx);
	*line = sbuffer + idx * tty_width;
	return rt;
}

static size_t string_to_buffer(
		uint16_t** line, size_t* p_x, uint8_t lmrg, 
		size_t len, const char* str, uint8_t clr) {

	size_t rt = 0;
	size_t x  = *p_x;
	for (; x < lmrg; ++x)
		(*line)[x] = vga_entry(' ', clr);
	for (size_t i = 0; i < len; ++x, ++i) {
        if (x >= tty_width) {
			rt += new_buffer_line(line);
			for (x = 0; x < lmrg; ++x)
				(*line)[x] = vga_entry(' ', clr);
        }
		(*line)[x] = vga_entry(str[i], clr);
    }
	*p_x = x;
	return rt;
}

size_t tty_prompt_to_buffer(size_t in_len) {
    size_t x = 0;
	uint16_t* line;
    size_t rt = new_buffer_line(&line);
	rt += string_to_buffer(&line, &x, 0, 
							input_msg_len, input_msg, vprompt_color);
	rt += string_to_buffer(&line, &x, input_lmargin, 
							in_len, ibuffer, input_color);
    for(; x < tty_width; ++x)
        line[x] = vga_entry(' ', input_color);
	tty_force_new_line();
	return rt;
}




size_t tty_buffer_cur_idx(){
    return cur_ln_x < tty_width
			? sb_ashift + sb_nb_lines - 1
			: sb_ashift + sb_nb_lines;
}

size_t tty_buffer_next_idx(){
    return sb_ashift + sb_nb_lines;
}

size_t tty_new_buffer_line_idx(size_t* index) {
    *index = (sb_ashift + sb_nb_lines) %tty_sb_height;
    if (sb_nb_lines == tty_sb_height - 1) {
        sb_ashift = (sb_ashift + 1) %tty_sb_height;
        if (sb_dtcd && sb_display_shift) {
            --sb_display_shift;
            return 0;
        }
        return 1;
    }
    ++sb_nb_lines;
    if (sb_sc_ed() > tty_height - input_height && !sb_dtcd) {
        ++sb_display_shift;
        return 1;
    }
    return 0;
}

void tty_force_new_line() {
    cur_ln_x = tty_width;
}

size_t tty_writestringl(const char* str, size_t len, uint8_t color) {
    const char* end = str + len;
    size_t rt = 0;
    char c = *str;
    size_t x = cur_ln_x;
    size_t ln_index = (sb_ashift + sb_nb_lines - 1) %tty_sb_height;
	uint16_t* line = sbuffer + ln_index * tty_width;

    goto loop_enter; while (str != end) {
        rt += new_buffer_line(&line);
        x = 0;
loop_enter:
        for(; x < tty_width && str!=end; ++x, c = *(++str)) {
            if(c == '\n') {
                for(;x < tty_width; ++x)
                    line[x] = vga_entry(' ', color);
                c = *(++str);
                break;
            } else if (c == '\t') {
                for(size_t i = 0; i < 4 && x < tty_width; ++x, ++i)
                    line[x] = vga_entry(' ', color);
                c = *(++str);
                break;
            }
            line[x] = vga_entry(c, color);
        }
    }
    cur_ln_x = x;
    for(; x < tty_width; ++x)
        line[x] = vga_entry(' ', color);
    return rt;
}

void tty_writer(void* shift, const char *str) {
    *((size_t*)shift) = tty_writestringl(str, strlen(str), tty_def_color);
}
void tty_seq_write(void* seq, const char* s, size_t len) {
    ((tty_seq_t*)seq)->shift += tty_writestringl(s, len, tty_def_color);
}

size_t tty_writei(uint8_t num, const char* str, size_t len) {
	tty_seq_t sq;
	tty_seq_init(&sq);
	if (num == 1) {
		size_t t_bg = 0;
		for (size_t i = 0; i < len; ++i) {
			if (str[i] == '\033') {
				if (t_bg < i)
					sq.shift += tty_writestringl(
									str + t_bg, i - t_bg, buf_color);
				size_t t_ed = i;
				if (++i >= len) {
					tty_seq_commit(&sq);
					return t_ed;
				}
				switch (str[i]) {
					case 'd':
						input_msg_len = 0;
						tty_set_bmode(ttym_def);
						break;
					case 'p':
						for (++i; i < len; ++i) {
							if (str[i] == '\033'
									&& ++i < len && str[i]==';')
								goto set_p_mode;
						}
						tty_seq_commit(&sq);
						return t_ed;
					set_p_mode:
						input_msg_len = min_uint16_t(IB_LENGTH,
											(i - 1) - (t_ed + 2));
						memcpy(input_msg, str + t_ed + 2, input_msg_len);
						tty_set_bmode(ttym_prompt);
						break;
				}
				t_bg = i + 1;
			}
		}
		if (t_bg < len)
			sq.shift += tty_writestringl(str + t_bg, len - t_bg, buf_color);
	} else
		sq.shift += tty_writestringl(str, len, err_color);
	tty_seq_commit(&sq);
	return len;
}
