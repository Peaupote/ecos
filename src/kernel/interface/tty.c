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
#include <kernel/param.h>

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
static char     ibuffer[IB_LENGTH];//ibuffer[ib_size] = ' '
static size_t   ib_size;
static int      ib_printed;// -1 if input_msg not printed
static size_t   input_height;
static size_t   input_top_line;
static size_t   input_cursor;
static uint8_t  cursor_clock;

static char     input_msg[IM_LENGTH];
static uint16_t input_msg_len;
static uint16_t input_msg_cs;
static uint8_t  input_lmargin;
static size_t   input_width;
static size_t   input_min_height;

static uint8_t  input_color, prompt_color, vprompt_color,
                buf_color,   err_color;
static bool     is_buf_bright = false;
uint8_t         tty_def_color;

static enum tty_mode tty_mode;

static enum tty_mode tty_bmode;
static char     input_msg_b[IM_LENGTH];
static uint16_t input_msg_len_b;
static char     promptb_color;

bool            tty_do_kprint = false;

size_t          tty_width, tty_height, tty_sb_height;

static size_t   cur_ln_x;

static int      buf_all_clock = 0;
static int      buf_all_st    = 0;

static tty_pos_t tty_wcurs = {.x = ~(size_t)0, .y = ~(size_t)0};
static tty_pos_t tty_dcurs = {.x = ~(size_t)0, .y = ~(size_t)0};
static bool display_buf = true;
static enum {CRS_NO, CRS_IN, CRS_SC} cursor_ty = CRS_NO;
static uint16_t cursor_sc_char = 0;


// --Affichage--

static bool is_in_screen(tty_pos_t* p) {
	return p->x < tty_width && p->y < tty_height;
}

static inline void putat(char c, uint8_t col, tty_pos_t pos) {
    display_echar_at(pos.x, pos.y, vga_entry(c, col));
}
static inline void puteat(uint16_t e, size_t x, size_t y) {
    display_echar_at(x, y, e);
}

static tty_pos_t afficher_string(tty_pos_t pos, uint8_t lmrg,
        size_t len, const char* str, uint8_t clr) {

    for (; pos.x < lmrg; ++pos.x)
        putat(' ', clr, pos);
    for (size_t i = 0; i < len; ++pos.x, ++i) {
        if (pos.x >= tty_width) {
            ++pos.y;
            for (pos.x = 0; pos.x < lmrg; ++pos.x)
                putat(' ', clr, pos);
        }
        putat(str[i], clr, pos);
    }
    return pos;
}

static const uint8_t code_to_vga[2][8] = {
		{
			VGA_COLOR_BLACK,
			VGA_COLOR_RED,
		 	VGA_COLOR_GREEN,
		 	VGA_COLOR_BROWN,
		 	VGA_COLOR_BLUE,
		 	VGA_COLOR_MAGENTA,
		 	VGA_COLOR_CYAN,
		 	VGA_COLOR_LIGHT_GREY
		 },
		{
			VGA_COLOR_BLACK,
			VGA_COLOR_LIGHT_RED,
		 	VGA_COLOR_LIGHT_GREEN,
		 	VGA_COLOR_LIGHT_BROWN,
		 	VGA_COLOR_LIGHT_BLUE,
		 	VGA_COLOR_LIGHT_MAGENTA,
		 	VGA_COLOR_LIGHT_CYAN,
		 	VGA_COLOR_WHITE
		 }
	};

static void interpret_color_code(const char* str, size_t len) {
	if (len == 1) {
		switch (str[0]) {
			case '0':
				is_buf_bright = false;
				buf_color = tty_def_color;
			break;
			case '1': is_buf_bright = true; break;
		}
	} else if (len == 2) {
		if ((str[0] == '3' || str[0] == '4') 
				&& (str[1] >= '0' || str[1] <= '7')) {
			uint8_t cl = code_to_vga[is_buf_bright ? 1 : 0][str[1] - '0'];
			if (str[0] == '3') {
				buf_color &= 0xf0;
				buf_color |= cl;
			} else {
				buf_color &= 0x0f;
				buf_color |= cl << 4;
			}
		}
	}
}

static uint8_t prompt_color_of_char(char c, bool bright) {
	return vga_entry_color(
			('0' <= c && c <= '7')
			? code_to_vga[bright ? 1 : 0][c - '0']
			: VGA_COLOR_LIGHT_GREY,
			VGA_COLOR_BLACK);
}


// --Screen Buffer--

static inline size_t sb_rel_im_bg() {
    return sb_display_shift;
}
static inline size_t sb_rel_im_ed() {
    return min_size_t(sb_nb_lines,
            (tty_height - input_height + sb_display_shift)%tty_sb_height);
}
size_t sb_new_botlim(size_t bt) {
    if (sb_dtcd) return bt > input_top_line ? 0x10000 : 0;
    size_t old_ds = sb_display_shift;
    sb_display_shift = bt < sb_nb_lines
            		 ? sb_nb_lines - bt : 0;
    return sb_display_shift - old_ds;
}
static inline size_t sb_sc_ed() {
    return sb_nb_lines - sb_display_shift;
}

void tty_afficher_buffer_range(
		size_t idx_bg, size_t x_bg,
		size_t idx_ed,   size_t x_ed) {

	if (!display_buf) return;

	size_t r_b = 0;
	if (idx_bg > sb_ashift)
		 r_b = (idx_bg - sb_ashift)%tty_sb_height;
	else x_bg = 0;
	size_t ri_bg = sb_rel_im_bg();
	if (ri_bg > r_b) {
		r_b  = ri_bg;
		x_bg = 0;
	}
	
	size_t r_e = 0;
	if (idx_ed > sb_ashift)
    	 r_e = (idx_ed - sb_ashift)%tty_sb_height;
	else return;
	size_t ri_ed = sb_rel_im_ed();
	if (ri_ed < r_e) {
		r_e  = ri_ed;
		x_ed = tty_width;
	}

	if (r_e == r_b + 1) {
		uint16_t* line = sbuffer
					   + ((sb_ashift + r_b)%tty_sb_height) * tty_width;
		for (size_t x = x_bg; x < x_ed; ++x) 
            puteat(line[x], x, r_b - sb_display_shift);
	} else if (r_e > r_b){
		size_t it = r_b;
		size_t x  = x_bg;
		goto loop_enter;
		for (; it < r_e; ++it) {
			uint16_t* line;
			x = 0;
		loop_enter:	
			line = sbuffer + ((sb_ashift + it)%tty_sb_height)*tty_width;
			for (; x < tty_width; ++x)
				puteat(line[x], x, it - sb_display_shift);
		}
    }
}

static void tty_afficher_buffer_all_do() {
	if (!display_buf) return;

    size_t r_b = sb_rel_im_bg();
    size_t r_e = sb_rel_im_ed();

    for (size_t it = r_b; it < r_e; ++it) {
        uint16_t* line = sbuffer + ((sb_ashift + it)%tty_sb_height)*tty_width;
        for (size_t x = 0; x < tty_width; ++x)
            puteat(line[x], x, it - sb_display_shift);
    }
}

void tty_afficher_buffer_all() {
	if (buf_all_st >= 1) {
		buf_all_st = 2;
		return;
	}
	tty_afficher_buffer_all_do();
	buf_all_st = 1;
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

size_t tty_writestringl(const char* str, size_t len, uint8_t color,
		uint8_t ln_color) {
    const char* end = str + len;
    size_t rt = 0;
    char c = *str;
    size_t x = cur_ln_x;
    size_t ln_index = (sb_ashift + sb_nb_lines - 1) %tty_sb_height;
    uint16_t* line = sbuffer + ln_index * tty_width;

	if (str == end) return rt;
    goto loop_enter; while (str != end) {
        rt += new_buffer_line(&line);
        x = 0;
loop_enter:
        for(; x < tty_width && str!=end; c = *(++str)) {
            if(c == '\n') {
                for(;x < tty_width; ++x)
                    line[x] = vga_entry(' ', ln_color);
                c = *(++str);
                break;
            } else if (c == '\t')
				do {
					line[x++] = vga_entry(' ', color);
				} while(x < tty_width && x % 4 != 0);
            else
				line[x++] = vga_entry(c, color);
        }
    }
    cur_ln_x = x;
    for(; x < tty_width; ++x)
        line[x] = vga_entry(' ', color);
    return rt;
}

void tty_writer(void* shift, const char *str) {
    *((size_t*)shift) = tty_writestringl(str, strlen(str),
							tty_def_color, tty_def_color);
}

void tty_seq_init(tty_seq_t* s) {
	s->idx_bg = sb_ashift + sb_nb_lines - 1;
	s->x_bg   = cur_ln_x;
    s->shift  = 0;
}

void tty_seq_write(void* seq, const char* s, size_t len) {
    ((tty_seq_t*)seq)->shift += tty_writestringl(s, len,
									tty_def_color, tty_def_color);
}

void tty_seq_commit(tty_seq_t* s) {
    s->shift += tty_update_prompt();
    if (s->shift) tty_afficher_buffer_all();
    else tty_afficher_buffer_range(s->idx_bg, s->x_bg,
								   sb_ashift + sb_nb_lines, cur_ln_x);
}


// --Prompt--

static inline size_t tty_input_to_buffer(bool nl) {
	size_t rt = 0;
    if (tty_mode == ttym_def) {
		if (!ib_size) {
			if (!nl) return 0;
			size_t idx;
			return tty_new_buffer_line_idx(&idx);
		}
        rt = tty_writestringl(ibuffer, ib_size, tty_def_color, tty_def_color);
	} else
        rt = tty_prompt_to_buffer(ib_size);
	if (nl) tty_force_new_line();
	return rt;
}

tty_pos_t tty_input_at(size_t p) {
	p += input_msg_cs;
	tty_pos_t rt = {
		.x = input_lmargin  + p % input_width,
		.y = input_top_line + p / input_width
	};
	return rt;
}

static void set_input_cursor(size_t nc) {
	if (display_graph || !~input_top_line) return;

	if (~input_cursor) {
		tty_pos_t cpos = tty_input_at(input_cursor);
		putat(ibuffer[input_cursor], input_color, cpos);
	}
	if (~nc) cursor_clock = 0;
	input_cursor = nc;
}

static inline size_t get_input_height () {
    size_t new_input_height =
        (ib_size + input_msg_cs + 1 + input_width-1) / input_width;
    return max_size_t(input_min_height, new_input_height);
}

size_t tty_update_prompt() {
	if (!~input_top_line) return 0;
	size_t      rt = 0;
	size_t nheight = get_input_height();
	size_t    ntop = sb_sc_ed();
	if (nheight != input_height || ntop != input_top_line) {
		input_height = nheight;

		if (ntop > tty_height - nheight)
			ntop = tty_height - nheight;

		if (ntop != input_top_line) {
			rt = sb_new_botlim(ntop);
			input_top_line = ntop;
			ib_printed     = -1;
		}

		tty_afficher_prompt(true);
	} else
		tty_afficher_prompt(false);
	return rt;
}

size_t tty_refresh_prompt() {
	ib_printed = -1;
	return tty_update_prompt();
}

size_t tty_new_prompt() {
	input_top_line = tty_height;
	ib_size      = 0;
	ibuffer[0]   = ' ';
	input_cursor = 0;
	return tty_refresh_prompt();
}

void tty_afficher_prompt(bool fill) {
	if (display_graph || !~input_top_line) return;

    tty_pos_t pos = {.x = 0, .y = input_top_line};
	if (ib_printed < 0) {
    	pos = afficher_string(pos, 0, input_msg_len, input_msg, prompt_color);
		ib_printed = 0;
		fill = true;
	} else {
		pos.x = (ib_printed + input_msg_cs) % input_width;
        pos.y = input_top_line + (ib_printed + input_msg_cs) / input_width;
		if (pos.x || !ib_printed)
			pos.x += input_lmargin;
	}

    pos = afficher_string(pos, input_lmargin, ib_size + 1 - ib_printed,
                            ibuffer + ib_printed, input_color);
    if(fill)
        for(; pos.x < tty_width; ++pos.x)
            putat(' ', input_color, pos);

    ib_printed = ib_size;

	if (cursor_clock < TTY_CURSOR_T / 2 && cursor_ty == CRS_IN) {
		tty_pos_t cpos = tty_input_at(input_cursor);
		display_cursor_at(cpos.x, cpos.y,
				vga_entry(ibuffer[input_cursor], input_color));
	}
}

static void tty_erase_prompt() {
	if (display_graph || !~input_top_line) return;

    for (tty_pos_t pos = {.y = input_top_line};
			pos.y < input_top_line + input_height; ++pos.y)
		for(pos.x = 0; pos.x < tty_width; ++pos.x)
			putat(' ', input_color, pos);
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


// --Mode--

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

    ib_printed     = -1;
	ib_size        = 0;
    input_height   = 0;
    input_top_line = ~(size_t)0;
	input_cursor   = ~(size_t)0;

    sb_ashift   = 0;
    sb_display_shift = 0;
    sb_nb_lines = 0;
    sb_dtcd     = false;

    tty_force_new_line();

	input_msg_len_b = 0;
	tty_bmode = m;
    tty_set_mode(m);
}

void tty_set_mode(enum tty_mode m) {
	cursor_clock = 0;
    switch (m) {
    case ttym_prompt:
        input_lmargin = 2;
        break;
    case ttym_def:
        input_lmargin = 0;
        break;
	case ttym_live:
		tty_mode       = ttym_live;
		tty_erase_prompt();
		cursor_ty      = CRS_SC;
		input_top_line =  ~(size_t)0;
		display_buf    = false;
		return;
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

	if (!~input_cursor) input_cursor = 0;
	cursor_ty   = CRS_IN;
	display_buf = !display_graph;
	if (tty_mode == ttym_live && !display_graph) {
		display_clear();
		tty_afficher_all();
	}
    tty_mode = m;

    if (~input_top_line) {
		tty_erase_prompt();
		if (tty_refresh_prompt())
			tty_afficher_buffer_all();
	} else if(tty_new_prompt())
		tty_afficher_all();
}

bool tty_display_graph_rq(bool g) {
	if (g) {
		if (tty_mode >= ttym_debug) return false;
		cursor_ty   = CRS_NO;
		display_buf = false;
		return true;
	} else {
		tty_set_mode(tty_mode);
		tty_afficher_all();
		return true;
	}
}

void tty_set_mode_to_b() {
	memcpy(input_msg, input_msg_b, input_msg_len_b);
	input_msg_len = input_msg_len_b;
	prompt_color  = prompt_color_of_char(promptb_color, true);
	vprompt_color = prompt_color_of_char(promptb_color, false);
	tty_set_mode(tty_bmode);
}

void tty_set_bmode(enum tty_mode m, uint16_t msg_len, const char* msg) {
	if ((tty_bmode == ttym_live) != (m == ttym_live))
		fs_proc_clear_tty0();

	tty_bmode = m;
	memcpy(input_msg_b, msg, msg_len);
	input_msg_len_b = msg_len;

	if (tty_mode < ttym_debug)
		tty_set_mode_to_b();
}

static void on_tty0_action() {
	if (tty_bmode == ttym_prompt)
		tty_set_bmode(ttym_def, 0, NULL);
}

void tty_afficher_all() {
	tty_afficher_buffer_all_do();
	tty_afficher_prompt(true);
}

static inline void tty_send_tty0(char* src, size_t len) {
	if(fs_proc_write_tty(src, len) != len)
		klogf(Log_error, "tty", "tty0 buffer saturated");
}

bool tty_input(scancode_byte s, key_event ev) {
    tty_seq_t sq;
    tty_seq_init(&sq);
	bool rt = false;

	if ((ev.flags & (KEY_FLAG_MODS | KEY_FLAG_PRESSED))
			== (KEY_FLAG_CTRL | KEY_FLAG_PRESSED)) {
		switch (ev.key) {
			case KEY_TAB:
				switch (tty_mode) {
					case ttym_def: case ttym_prompt: case ttym_live:
						display_set_debug(true);
						tty_set_mode(ttym_debug);
					break;
					case ttym_debug:
						tty_set_mode_to_b();
						display_set_debug(false);
					break;
					case ttym_panic:  break;
				}
			return false;
			case KEY_C:
				if (~state.st_owng[own_tty]) {
					sq.shift += tty_writestringl("^C", 2,
									tty_def_color, tty_def_color);
					send_sig_to_proc(state.st_owng[own_tty], SIGINT - 1);
					on_tty0_action();
					rt = true;
				}
			goto end_input;
			case KEY_Z:
				if (~state.st_owng[own_tty]) {
					sq.shift += tty_writestringl("^Z", 2,
									tty_def_color, tty_def_color);
					send_sig_to_proc(state.st_owng[own_tty], SIGTSTP - 1);
					on_tty0_action();
					rt = true;
				}
			goto end_input;
			case KEY_D:
				if (!~input_top_line) break;
				if (ib_size) {
					sq.shift += tty_input_to_buffer(false);
					tty_send_tty0(ibuffer, ib_size);
					sq.shift += tty_new_prompt();
				} else {
					sq.shift += tty_writestringl("^D", 2,
									tty_def_color, tty_def_color);
					fs_proc_send0_tty();
				}
				on_tty0_action();
				rt = true;
			goto end_input;
		}
	}

	if (tty_mode == ttym_live) {
		if (!ev.key) return false;
		tty_live_t lt;
		lt.magic    = TTY_LIVE_MAGIC;
		lt.ev.key   = ev.key;
		lt.ev.ascii = ev.ascii;
		lt.ev.flags = ev.flags;
		tty_send_tty0((char*)&lt, sizeof(lt));
		return true;
	}

    if (tty_do_kprint)
		tty_seq_printf(&sq, "s=%x c=%x f=%x p=%c\n",
				(int)s, (int)ev.key, (int)ev.flags,
				0x20 <= ev.ascii && ev.ascii <= 0x7e
				? ev.ascii : '_');

    if (ev.key && (ev.flags & KEY_FLAG_PRESSED)) {
		switch (ev.key) {
		case KEYS_ENTER: case KEY_KP_ENTER:
            sq.shift += tty_input_to_buffer(true);

            switch(tty_mode) {
            case ttym_def:
            case ttym_prompt:
                ibuffer[ib_size] = '\n';
				tty_send_tty0(ibuffer, ib_size + 1);
				on_tty0_action();
				rt = true;
                break;
            default:
                ibuffer[ib_size] = '\0';
                tty_built_in_exec(&sq, ibuffer);
                break;
            }

            sq.shift += tty_new_prompt();
		break;
		case KEY_BACKSPACE:
            if(input_cursor > 0) {
                if (display_graph || !~input_top_line)
					--ib_size;
				else {
					putat(' ', input_color, tty_input_at(ib_size));
					--ib_size;
					putat(' ', input_color, tty_input_at(ib_size));
				}
				for (size_t i = input_cursor-1; i < ib_size; ++i)
					ibuffer[i] = ibuffer[i + 1];
				mina_int(&ib_printed, input_cursor-1);
				ibuffer[ib_size] = ' ';
				set_input_cursor(input_cursor-1);
            }
        break;
		case KEY_UP_ARROW:
            if (sb_display_shift) {
                size_t mv = ev.flags & KEY_FLAG_SHIFT
                          ? (tty_height - input_height) : 1;
                if (mv > sb_display_shift)
                    sb_display_shift = 0;
                else
                    sb_display_shift -= mv;
                sb_dtcd = true;
				sq.shift -= mv;
            }
		break;
		case KEY_DOWN_ARROW:
            if (sb_sc_ed() > tty_height - input_height) {
                size_t mv = ev.flags & KEY_FLAG_SHIFT
                          ? (tty_height - input_height) : 1;
                mina_size_t(&mv,
                        sb_sc_ed() - (tty_height - input_height));
                sb_display_shift += mv;
                if (sb_sc_ed() == tty_height - input_height)
                    sb_dtcd = false;
                sq.shift += mv;
            }
        break;
		case KEY_LEFT_ARROW:
			if (input_cursor > 0) {
				if (ev.flags & KEY_FLAG_SHIFT)
					set_input_cursor(0);
				else if (ev.flags & KEY_FLAG_CTRL) {
					size_t curs = input_cursor - 1;
					while (curs > 0 && ibuffer[  curs  ] == ' ') --curs;
					while (curs > 0 && ibuffer[curs - 1] != ' ') --curs;
					set_input_cursor(curs);
				} else
					set_input_cursor(input_cursor - 1);
			}
		break;
		case KEY_RIGHT_ARROW:
			if (input_cursor < ib_size) {
				if (ev.flags & KEY_FLAG_SHIFT)
					set_input_cursor(ib_size);
				else if (ev.flags & KEY_FLAG_CTRL) {
					size_t curs = input_cursor;
					while (curs < ib_size && ibuffer[curs] != ' ') ++curs;
					while (curs < ib_size && ibuffer[curs] == ' ') ++curs;
					set_input_cursor(curs);
				} else
					set_input_cursor(input_cursor + 1);
			}
		break;
		default:
			if (ev.flags & (KEY_FLAG_CTRL | KEY_FLAG_ALT))
				break;
			char kchar = ev.ascii;
			if (0x20 <= kchar && kchar <= 0x7e && ib_size < IB_LENGTH - 1) {
				mina_int(&ib_printed, input_cursor);
				for (size_t i = ib_size; i > input_cursor; --i)
					ibuffer[i] = ibuffer[i-1];
				ibuffer[input_cursor] = kchar;
				ibuffer[++ib_size]    = ' ';
				set_input_cursor(input_cursor+1);
			}
		break;
		}
    }
end_input:
    tty_seq_commit(&sq);
	return rt;
}

void tty_on_pit() {
	if (display_graph) return;
	cursor_clock = (cursor_clock + 1) % TTY_CURSOR_T;
	if (cursor_ty == CRS_IN) {
		if (cursor_clock == 0) {
			tty_pos_t cpos = tty_input_at(input_cursor);
			display_cursor_at(cpos.x, cpos.y,
					vga_entry(ibuffer[input_cursor], input_color));
		} else if (cursor_clock == TTY_CURSOR_T / 2) {
			tty_pos_t cpos = tty_input_at(input_cursor);
			putat(ibuffer[input_cursor], input_color, cpos);
		}
	} else if (cursor_ty == CRS_SC && is_in_screen(&tty_dcurs)) {
		if (cursor_clock == 0)
			display_cursor_at(tty_dcurs.x, tty_dcurs.y, cursor_sc_char);
		else if (cursor_clock == TTY_CURSOR_T / 2)
			puteat(cursor_sc_char, tty_dcurs.x, tty_dcurs.y);
	}
	if (display_buf && ++buf_all_clock >= TTY_BUFALL_T) {
		buf_all_clock = 0;
		if (buf_all_st == 2)
			tty_afficher_buffer_all_do();
		buf_all_st = 0;
	}
}

void tty_set_owner(pid_t p) {
    klogf(Log_info, "tty", "tty owner is %d", (int)p);
    state.st_owng[own_tty] = p;
}

static size_t tty_writestringl0(const char* str, size_t len) {
	if (tty_mode == ttym_live) {
		if (display_graph) return 0;
		for (size_t i = 0; i < len; ++i) {
			if (tty_wcurs.x >= tty_width) {
				tty_wcurs.x = 0;
				++tty_wcurs.y;
			}
			if (tty_wcurs.y >= tty_height)
				tty_wcurs.y = 0;
			putat(str[i], buf_color, tty_wcurs);
			++tty_wcurs.x;
		}
		return 0;
	} else
		return tty_writestringl(str, len, buf_color, tty_def_color);
}

static bool parse_cursor(const char* str, size_t* i, size_t len,
		unsigned* rt) {
	bool is_y = false;
	rt[0] = rt[1] = 0;
	for (; *i < len; ++*i) {
		if (str[*i] == ';') {
			if (is_y) return true;
			is_y = true;
			++rt;
		} else if ('0' <= str[*i] && str[*i] <= '9')
			*rt = (*rt) * 10 + str[*i] - '0';
		else {
			klogf(Log_error, "tty", "fail on %c", str[*i]);
			return false;
		}
	}
	klogf(Log_error, "tty", "fail len");
	return false;
}

size_t tty_writei(uint8_t num, const char* str, size_t len) {
	tty_seq_t sq;
	tty_seq_init(&sq);
	if (num == 1) {
		size_t t_bg = 0;
		for (size_t i = 0; i < len; ++i) {
			if (str[i] == '\033') {
				if (t_bg < i)
					sq.shift += tty_writestringl0(str + t_bg, i - t_bg);
				size_t t_ed = i;
				if (++i >= len) {
					tty_seq_commit(&sq);
					return t_ed;
				}
				switch (str[i]) {
					case 'd':
						tty_set_bmode(ttym_def, 0, NULL);
						break;
					case 'l':
						tty_set_bmode(ttym_live, 0, NULL);
						break;
					case 'p': {
						char pcl;
						if (i + 1 < len) {
							pcl = str[++i];
							for (++i; i < len; ++i) {
								if (str[i] == '\033'
										&& ++i < len && str[i]==';')
									goto set_p_mode;
							}
						}
						tty_seq_commit(&sq);
						return t_ed;
					set_p_mode:
						promptb_color = pcl;
						tty_set_bmode(ttym_prompt,
								min_uint16_t(IB_LENGTH, (i - 1) - (t_ed + 3)),
								str + t_ed + 3);
					} break;
					case '\n':
						if (cur_ln_x) tty_force_new_line();
					break;
					case '[':
						for (int cbg = ++i; i < len; ++i) {
							if (str[i] == 'm') {
								interpret_color_code(str + cbg, i - cbg);
								goto end_eseq;
							} else if (str[i] == ';') {
								interpret_color_code(str + cbg, i - cbg);
								cbg = i + 1;
							}
						}
						tty_seq_commit(&sq);
						return t_ed;
					case 'c': {
						unsigned pos[2];
						++i;
						if (parse_cursor(str, &i, len, pos)) {
							tty_wcurs.x = pos[0];
							tty_wcurs.y = pos[1];
							goto end_eseq;
						} else {
							tty_seq_commit(&sq);
							return t_ed;
						}
					}
					case 'C': {
						unsigned pos[2];
						char nc;
						if (++i < len && (nc=str[i++],
									parse_cursor(str, &i, len, pos))) {
							if (!display_graph)
								puteat(cursor_sc_char, tty_dcurs.x,
													   tty_dcurs.y);
							tty_dcurs.x = pos[0];
							tty_dcurs.y = pos[1];
							cursor_sc_char = vga_entry(nc, buf_color);
							cursor_clock = 0;
							if (is_in_screen(&tty_dcurs))
								puteat(cursor_sc_char, tty_dcurs.x,
										               tty_dcurs.y);
							goto end_eseq;
						} else {
							tty_seq_commit(&sq);
							return t_ed;
						}
					}
					case 'i': {
						char buf[50];
						int count = sprintf(buf, "i%u;%u;",
											(unsigned)tty_width,
											(unsigned)tty_height);
						for (int i = 0; i < count; ++i) {
							tty_live_t lt;
							lt.magic    = TTY_LIVE_MAGIC;
							lt.ev.key   = 0;
							lt.ev.ascii = buf[i];
							lt.ev.flags = 0;
							tty_send_tty0((char*)&lt, sizeof(lt));
						}
					} break;
					case 'x':
						if (tty_mode == ttym_live)
							display_clear();
						break;
				}
				end_eseq:
				t_bg = i + 1;
			}
		}
		if (t_bg < len)
			sq.shift += tty_writestringl0(str + t_bg, len - t_bg);
	} else
		sq.shift += tty_writestringl(str, len, err_color, tty_def_color);
	tty_seq_commit(&sq);
	return len;
}
