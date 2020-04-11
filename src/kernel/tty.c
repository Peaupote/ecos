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

#include <fs/proc.h>

#include <kernel/tests.h>
#include <fs/ext2.h>

#define SB_MASK  0x7f

#define IB_LENGTH 256

//Screen Buffer
// SB_HEIGHT       |.....|bbb|ddd|......|
// ashift          |---->|
// display_shift         |-->|
// nb_lines              |------>|
uint16_t sbuffer[SB_HEIGHT][80];
size_t   sb_ashift; //shift array
size_t   sb_display_shift; // <= sb_nb_lines
// < SB_HEIGHT pour éviter 0 = SB_HEIGHT lors du passage au modulo
size_t   sb_nb_lines;
bool     sb_dtcd; // scroll détaché

//Input Buffer
char     ibuffer[IB_LENGTH];
size_t   ib_size;
size_t   ib_printed;
const size_t input_width = 80 - 2;
size_t   input_height;
size_t   input_bottom_line;
size_t   input_top_line;

uint8_t  input_color;
uint8_t  prompt_color;
uint8_t  vprompt_color;
uint8_t  back_color;

static pid_t tty_owner = PID_NONE;

static inline size_t sb_rel_im_bg() {
    return sb_display_shift;
}
static inline size_t sb_rel_im_ed() {
    return min_size_t(sb_nb_lines,
            (VGA_HEIGHT - input_height + sb_display_shift) & SB_MASK);
}
size_t sb_new_in_height(size_t h) {
    if (sb_dtcd) return 0;
    size_t old_ds = sb_display_shift;
    size_t dsp_ln = VGA_HEIGHT - h;
    sb_display_shift = dsp_ln < sb_nb_lines
            ? sb_nb_lines - dsp_ln : 0;
    return sb_display_shift - old_ds;

}
static inline size_t sb_sc_ed() {
    return sb_nb_lines - sb_display_shift;
}

static enum tty_mode tty_mode;

void tty_init(enum tty_mode m) {
    back_color = vga_entry_color (
                VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    input_color = vga_entry_color (
                VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    ib_printed   = ib_size = 0;
    input_height = 0;
    input_bottom_line = input_top_line = ~0;

    sb_ashift   = 1;// evite que cur_idx renvoit -1
    sb_display_shift = 0;
    sb_nb_lines = 0;
    sb_dtcd     = false;

	tty_force_new_line();

    tty_set_mode(m);
}

void tty_set_mode(enum tty_mode m) {
    switch (m) {
    case ttym_def:
        prompt_color = vga_entry_color (
                    VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
        vprompt_color = vga_entry_color (
                    VGA_COLOR_BLUE, VGA_COLOR_BLACK);
        break;
    case ttym_panic:
        prompt_color = vga_entry_color (
                    VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vprompt_color = vga_entry_color (
                    VGA_COLOR_RED, VGA_COLOR_BLACK);
        break;

    case ttym_debug:
        prompt_color = vga_entry_color (
                    VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vprompt_color = vga_entry_color (
                    VGA_COLOR_GREEN, VGA_COLOR_BLACK);
        break;
    }

    tty_mode = m;
    if (~input_top_line)
        vga_putentryat('>', prompt_color, 0, input_top_line);
}

void tty_set_owner(pid_t p) {
    klogf(Log_info, "tty", "tty owner is %d", (int)p);
    tty_owner = p;
}

uint8_t do_kprint = 0;

void ls (char** tokpt) {
    char* arg1 = strtok_rnull(NULL, " ", tokpt);
    if (!arg1) return;

	vfile_t *vf = vfs_load(arg1, 0);
    if (!vf) {
        kprintf("%s don't exist\n", arg1);
        return;
    }

	chann_adt_t cdt;
	vfs_opench(vf, &cdt);

    char dbuf[512];

	struct device *dev = devices + vf->vf_stat.st_dev;
	struct fs *fs = fst + dev->dev_fs;
	struct stat st;

	kprintf("size %d\n", vf->vf_stat.st_size);
	kprintf("INO    TYPE    SIZE    NLINK        NAME\n");

	int rc;
	while((rc = vfs_getdents(vf, (struct dirent*)dbuf, 512, &cdt)) > 0) {
		struct dirent* de = (struct dirent*)dbuf;
		if (rc < de->d_rec_len) {
			klogf(Log_error, "ls", 
					"pas assez de place pour stocker le nom");
			break;
		}
		for (int i = 0; i < rc;
				i += de->d_rec_len, de = (struct dirent*)(dbuf + i)) {
			fs->fs_stat(de->d_ino, &st, &dev->dev_info);
			kprintf("%d    %x    %d    %d        %s\n",
					de->d_ino, st.st_mode, st.st_size, st.st_nlink,
					de->d_name);
		}
	}

    vfs_close(vf);
}

void inspect_vfiles() {
    kprintf("FILE    INO    DEV    CNT\n");
    for (size_t i = 0; i < NFILE; i++) {
        vfile_t *vf = state.st_files + i;
        if (vf->vf_cnt > 0)
            kprintf("%d    %d    %d    %d\n",
                    i, vf->vf_stat.st_ino, vf->vf_stat.st_dev, vf->vf_cnt);
    }
}

void inspect_channs() {
    kprintf("CID    VFILE    MODE    ACC\n");
    for (size_t i = 0; i < NCHAN; i++) {
        chann_t *c = state.st_chann + i;
        if (c->chann_acc > 0) {
            kprintf("%d    %d    %d    %d\n",
                    i, (c->chann_vfile - state.st_files) / sizeof(vfile_t),
                    c->chann_mode, c->chann_acc);
        }
    }

}

uint_ptr read_ptr(const char str[]) {
    if (str[0] == 'k')
        return int64_of_str_hexa(str+1)
            + paging_add_lvl(pgg_pml4, PML4_KERNEL_VIRT_ADDR);
    return int64_of_str_hexa(str);
}

/*
 * log [-options] [hd]
 * hd: filtre d'en-tête, '*' pour aucun filtre
 * options:
 *   -0 	aucun
 *   -e 	erreurs
 *   -w 	warnings
 *   -i 	infos
 *   -v		verbose
 *   -vv	very verbose
 * */
void change_log(char** tokpt) {
	char* arg;
	while ((arg = strtok_rnull(NULL, " ", tokpt))) {
		if (arg[0] == '-') {
			switch(arg[1]) {
				case 'e':
					klog_level = Log_error;
					break;
				case 'w':
					klog_level = Log_warn;
					break;
				case 'i':
					klog_level = Log_info;
					break;
				case 'v':
					klog_level = arg[2] == 'v' ? Log_vverb : Log_verb;
					break;
				default:
					klog_level = Log_none;
			}
		} else if (!strcmp(arg, "*"))
			*klog_filtr = '\0';
		else if (strlen(arg) < 256)
			strcpy(klog_filtr, arg);
	}
}

size_t built_in_exec() {
    char* tokpt;
    char* cmd_name = strtok_rnull(ibuffer, " ", &tokpt);
    if (!cmd_name) return 0;
    if (!strcmp(cmd_name, "tprint"))
        return tty_writestring("test print");
    else if (!strcmp(cmd_name, "memat")) {
        char* arg1 = strtok_rnull(NULL, " ", &tokpt);
        if (!arg1) return 0;
        uint_ptr ptr = read_ptr(arg1);
        char data_str[3];
        data_str[2] = '\0';
        int8_to_str_hexa(data_str, *(uint8_t*)ptr);
        return tty_writestring(data_str);
    } else if (!strcmp(cmd_name, "pg2")) {
        char* arg1 = strtok_rnull(NULL, " ", &tokpt);
        if (!arg1) return 0;
        uint_ptr ptr = read_ptr(arg1);
        kmem_print_paging(ptr);
    } else if (!strcmp(cmd_name, "kprint"))
        do_kprint = !do_kprint;
    else if (!strcmp(cmd_name, "a"))
        use_azerty = 1;
    else if (!strcmp(cmd_name, "q"))
        use_azerty = 0;
    else if (!strcmp(cmd_name, "test")) {
        char* arg1 = strtok_rnull(NULL, " ", &tokpt);
        if (!arg1) return 0;
        if (!strcmp(arg1, "statut"))      test_print_statut();
        else if(!strcmp(arg1, "kheap"))   test_kheap();
    } else if (!strcmp(cmd_name, "ps"))
        proc_ps();
    else if (!strcmp(cmd_name, "unblock")) {
        char* arg1 = strtok_rnull(NULL, " ", &tokpt);
        if (!arg1) return 0;
        char* arg2 = strtok_rnull(NULL, " ", &tokpt);
        if (!arg2) return 0;
        int   ipid, val = 0;
        sscanf(arg1, "%d", &ipid);
        pid_t pid = ipid;
        sscanf(arg2, "%i", &val);
        state.st_proc[pid].p_reg.r.rdi.i = val;
        proc_unblock_1(pid, state.st_proc + pid);
    } else if (!strcmp(cmd_name, "ls")) ls(&tokpt);
    else if (!strcmp(cmd_name, "kill")) {
        char* arg1 = strtok_rnull(NULL, " ", &tokpt);
        if (!arg1) return 0;
        char* arg2 = strtok_rnull(NULL, " ", &tokpt);
        if (!arg2) return 0;
        int   ipid, signum = 0;
        sscanf(arg1, "%d", &ipid);
        pid_t pid = ipid;
        sscanf(arg2, "%i", &signum);

        pid_t cproc = state.st_curr_pid;
        bool spr = pid == state.st_curr_pid;
        bool afc =  1  == send_sig_to_proc(pid, signum - 1);
        if (spr && afc)
            klogf(Log_error, "warning", "processus courant");
        if (cproc != state.st_curr_pid)
            switch_proc(cproc);
    }
    else if (!strcmp(cmd_name, "vfiles")) inspect_vfiles();
    else if (!strcmp(cmd_name, "channs")) inspect_channs();
	else if (!strcmp(cmd_name, "log"))    change_log(&tokpt);

    return 0;
}

void tty_input(scancode_byte s, key_event ev) {
    tty_seq_t sq;
    tty_seq_init(&sq);
    uint8_t p_updt = 0;

    if (do_kprint) {
        char str[] = "s=__ c=__ f=__ p=__\n";
        int8_to_str_hexa(str + 2,  s);
        int8_to_str_hexa(str + 7,  ev.key);
        int8_to_str_hexa(str + 12, ev.flags);
        int8_to_str_hexa(str + 17, keycode_to_printable(ev.key));
        p_updt = 1;
        sq.shift += tty_writestring(str);
        sq.shift += tty_update_prompt_pos();
    }
    if (ev.key && !(ev.flags & 0x1)) {//évènement appui
        if ((ev.key&KEYS_MASK) == KEYS_ENTER) {
            p_updt = 1;
            sq.shift += tty_prompt_to_buffer(ib_size);

            switch(tty_mode) {
            case ttym_def:
                ibuffer[ib_size] = '\n';
				if(fs_proc_write_tty(ibuffer, ib_size + 1) != ib_size + 1)
					klogf(Log_error, "tty", "in_buffer saturated");
                break;
            default:
                ibuffer[ib_size] = '\0';
                sq.shift += built_in_exec();
                break;
            }

            ib_size = ib_printed = 0;
            sq.shift += tty_new_prompt();
        } else if(ev.key == KEY_BACKSPACE) {
            if(ib_size > 0) {
                --ib_size;
                if(ib_printed > ib_size) ib_printed = ib_size;
                vga_putentryat(' ', input_color,
                        2 + ib_size % input_width,
                        input_top_line + ib_size / input_width);
            }
        } else if (ev.key == KEY_UP_ARROW) {
            if (sb_display_shift) {
                size_t mv = key_state_shift()
                          ? (VGA_HEIGHT - input_height) : 1;
                if (mv > sb_display_shift)
                    sb_display_shift = 0;
                else
                    sb_display_shift -= mv;
                sb_dtcd = true;
                tty_afficher_buffer_all();
            }
        } else if (ev.key == KEY_DOWN_ARROW) {
            if (sb_sc_ed() > VGA_HEIGHT - input_height) {
                size_t mv = key_state_shift()
                          ? (VGA_HEIGHT - input_height) : 1;
                mina_size_t(&mv,
                        sb_sc_ed() - (VGA_HEIGHT - input_height));
                sb_display_shift += mv;
                if (sb_sc_ed() == VGA_HEIGHT - input_height)
                    sb_dtcd = false;
                tty_afficher_buffer_all();
            }
        } else if (key_state_ctrl()) {
            switch (ev.key) {
                case KEY_TAB:
                switch (tty_mode) {
                    case ttym_def:   tty_set_mode(ttym_debug); break;
                    case ttym_debug: tty_set_mode(ttym_def);   break;
                    case ttym_panic: break;
                }
                break;
                case KEY_C:
                    if (~tty_owner && proc_alive(state.st_proc + tty_owner)) {
						kprintf("^C\n");
                        send_sig_to_proc(tty_owner, SIGINT - 1);
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
    if (p_updt) tty_seq_commit(&sq);
}

void tty_afficher_buffer_range(size_t idx_begin, size_t idx_end) {
    size_t r_b = idx_begin > sb_ashift 
			? ((idx_begin - sb_ashift) & SB_MASK)
			: 0;
    size_t r_e = (idx_end   - sb_ashift) & SB_MASK;
    maxa_size_t(&r_b, sb_rel_im_bg());
    mina_size_t(&r_e, sb_rel_im_ed());

    for (size_t it = r_b; it < r_e; ++it)
        for (size_t x = 0; x < VGA_WIDTH; ++x)
            vga_putcentryat(sbuffer[(sb_ashift + it) & SB_MASK][x], x,
                    it - sb_display_shift);
}

void tty_afficher_buffer_all() {
    size_t r_b = sb_rel_im_bg();
    size_t r_e = sb_rel_im_ed();

    for (size_t it = r_b; it < r_e; ++it)
        for (size_t x = 0; x < VGA_WIDTH; ++x)
            vga_putcentryat(sbuffer[(sb_ashift + it) & SB_MASK][x], x,
                    it - sb_display_shift);
}

size_t tty_new_prompt() {
    size_t rt = 0;
    input_height = 1;
    if (sb_sc_ed() >= VGA_HEIGHT) {
        input_top_line = VGA_HEIGHT - 1;
        rt = sb_new_in_height(1);
    } else
        input_top_line = sb_sc_ed();

    input_bottom_line = input_top_line;

    vga_putentryat('>', prompt_color, 0, input_top_line);
    for (size_t i=1; i<VGA_WIDTH; ++i)
        vga_putentryat(' ', input_color, i, input_top_line);

    return rt;
}

static inline size_t get_input_height () {
    size_t new_input_height = ib_size / input_width;
    if((ib_size % input_width) || !ib_size)
        return 1 + new_input_height;
    return new_input_height;
}

void tty_afficher_prompt_do(uint8_t fill) {
    vga_pos_t pos = {
            .x = 2 + ib_printed % input_width,
            .y = input_top_line + ib_printed / input_width
        };

    for (size_t i = ib_printed; i < ib_size; ++pos.x, ++i) {
        if (pos.x >= VGA_WIDTH) {
            ++pos.y;
            if(fill) {
                vga_putentryat(' ', input_color, 0, pos.y);
                vga_putentryat(' ', input_color, 1, pos.y);
            }
            pos.x = 2;
        }
        vga_putentryat(ibuffer[i],
                input_color, pos.x, pos.y);
    }

    if(fill)
        for(size_t x = pos.x; x < VGA_WIDTH; ++x)
            vga_putentryat(' ', input_color, x, pos.y);

    ib_printed = ib_size;
}

void tty_afficher_prompt_all() {
    vga_putentryat('>', prompt_color, 0, input_top_line);
    vga_putentryat(' ', prompt_color, 1, input_top_line);
    ib_printed = 0;
    tty_afficher_prompt_do(1);
}

void tty_afficher_prompt() {
    size_t new_input_height = get_input_height();
    size_t bt_line_0;

    if (new_input_height != input_height) {
        input_height = new_input_height;
        bt_line_0 = input_top_line + input_height - 1;

        if (bt_line_0 >= VGA_HEIGHT) {
            input_bottom_line = VGA_HEIGHT - 1;
            input_top_line = input_bottom_line - input_height + 1;
            sb_new_in_height(input_height);
            tty_afficher_buffer_all(); //scroll
            tty_afficher_prompt_all();
            return;
        }

        input_bottom_line = bt_line_0;
        tty_afficher_prompt_do(1);
        return;
    }
    tty_afficher_prompt_do(0);
}

size_t tty_update_prompt_pos() {
    if (~input_top_line) {
        size_t n_top;
        size_t rt = 0;
        if (sb_sc_ed() + input_height > VGA_HEIGHT) {
            n_top   = VGA_HEIGHT - input_height;
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

size_t tty_prompt_to_buffer(size_t in_len) {
    size_t index;
    size_t rt = tty_new_buffer_line(&index);
    size_t x = 2;
    size_t it = 0;

    size_t l_lim = in_len + 2;
    if(l_lim > VGA_WIDTH) {
        l_lim = VGA_WIDTH;
        in_len -= VGA_WIDTH - 2;
    } else in_len = 0;

    sbuffer[index][0] = vga_entry('>', vprompt_color);
    sbuffer[index][1] = vga_entry(' ', input_color);
    for (; x < l_lim; ++it, ++x)
        sbuffer[index][x] = vga_entry(ibuffer[it], input_color);

    while (in_len) {
        rt += tty_new_buffer_line(&index);
        l_lim = in_len + 2;
        if(l_lim > VGA_WIDTH){
            l_lim = VGA_WIDTH;
            in_len -= VGA_WIDTH - 2;
        } else in_len = 0;
        sbuffer[index][0] = vga_entry(' ', input_color);
        sbuffer[index][1] = vga_entry(' ', input_color);
        for (x=2; x < l_lim; ++it, ++x)
            sbuffer[index][x] = vga_entry(ibuffer[it], input_color);
    }

    for(; x < VGA_WIDTH; ++x)
        sbuffer[index][x] = vga_entry(' ', input_color);
    tty_force_new_line();
    return rt;
}

size_t cur_ln_x = 80;

size_t tty_buffer_cur_idx(){
    return cur_ln_x < VGA_WIDTH
			? sb_ashift + sb_nb_lines - 1
			: sb_ashift + sb_nb_lines;
}

size_t tty_buffer_next_idx(){
    return sb_ashift + sb_nb_lines;
}

size_t tty_new_buffer_line(size_t* index) {
    *index = (sb_ashift + sb_nb_lines) & SB_MASK;
    if (sb_nb_lines == SB_HEIGHT - 1) {
        sb_ashift = (sb_ashift + 1) & SB_MASK;
        if (sb_dtcd && sb_display_shift) {
            --sb_display_shift;
            return 0;
        }
        return 1;
    }
    ++sb_nb_lines;
    if (sb_sc_ed() > VGA_HEIGHT - input_height && !sb_dtcd) {
        ++sb_display_shift;
        return 1;
    }
    return 0;
}

void tty_force_new_line() {
    cur_ln_x = VGA_WIDTH;
}

size_t tty_writestring(const char* str) {
    size_t rt = 0;
    char c = *str;
    size_t x = cur_ln_x;
    size_t ln_index = (sb_ashift + sb_nb_lines - 1) & SB_MASK;
    goto loop_enter; while (c) {
        rt += tty_new_buffer_line(&ln_index);
        x = 0;
loop_enter:
        for(; x < VGA_WIDTH && c; ++x, c = *(++str)) {
            if(c == '\n') {
                for(;x < VGA_WIDTH; ++x)
                    sbuffer[ln_index][x] = vga_entry(' ', back_color);
                c = *(++str);
                break;
            }
            sbuffer[ln_index][x] = vga_entry(c, back_color);
        }
    }
    cur_ln_x = x;
    for(; x < VGA_WIDTH; ++x)
        sbuffer[ln_index][x] = vga_entry(' ', back_color);
    return rt;
}

size_t tty_writestringl(const char* str, size_t len) {
    const char* end = str + len;
    size_t rt = 0;
    char c = *str;
    size_t x = cur_ln_x;
    size_t ln_index = (sb_ashift + sb_nb_lines - 1) & SB_MASK;
    goto loop_enter; while (str != end) {
        rt += tty_new_buffer_line(&ln_index);
        x = 0;
loop_enter:
        for(; x < VGA_WIDTH && str!=end; ++x, c = *(++str)) {
            if(c == '\n') {
                for(;x < VGA_WIDTH; ++x)
                    sbuffer[ln_index][x] = vga_entry(' ', back_color);
                c = *(++str);
                break;
            } else if (c == '\t') {
                for(size_t i = 0; i < 4 && x < VGA_WIDTH; ++x, ++i)
                    sbuffer[ln_index][x] = vga_entry(' ', back_color);
                c = *(++str);
                break;
            }
            sbuffer[ln_index][x] = vga_entry(c, back_color);
        }
    }
    cur_ln_x = x;
    for(; x < VGA_WIDTH; ++x)
        sbuffer[ln_index][x] = vga_entry(' ', back_color);
    return rt;
}

void tty_writer(void* shift, const char *str) {
    *((size_t*)shift) = tty_writestring(str);
}
void tty_seq_write(void* seq, const char* s, size_t len) {
    ((tty_seq_t*)seq)->shift += tty_writestringl(s, len);
}
