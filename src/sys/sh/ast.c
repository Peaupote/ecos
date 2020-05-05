#include "sh.h"

#include <stdlib.h>
#include <string.h>

void destr_redir(redir_t* rd) {
	switch (rd->type) {
		case IN_FILE: case OUT_APPEND: case OUT_TRUNC:
			free(rd->filewd);
			break;
		case IN_DUP: case OUT_DUP: break;
	}
}

void destr_red_list(red_list_t* l) {
	for (size_t rd = 0; rd < l->redc; ++rd)
		destr_redir(l->reds + rd);
	free(l->reds);
	l->reds = NULL;
}

void destr_cmd_0(cmd_0_t* ptr) {
	switch (ptr->ty) {
		case C_BAS:
			free(ptr->bas);
			break;
		case C_SUB:
			if (ptr->sub) {
				destr_cmd_3(ptr->sub);
				free(ptr->sub);
				ptr->sub = NULL;
			}
			break;
	}
}

void destr_cmd_1(cmd_1_t* ptr) {
	destr_cmd_0(&ptr->c);
	destr_red_list(&ptr->r);
}

void destr_cmd_2(cmd_2_t* ptr) {
	for (size_t c = 0; c < ptr->cmdc; ++c)
		destr_cmd_1(ptr->cmds + c);
	free(ptr->cmds);
	ptr->cmds = NULL;
}

void destr_cmd_3(cmd_3_t* c) {
	switch (c->ty) {
		case C_CM2: destr_cmd_2(&c->cm2); break;
		case C_BG:
			destr_cmd_3(c->childs[0]);
			free(c->childs[0]);
			c->childs[0] = NULL;
		break;
		case C_SEQ: case C_AND: case C_OR:
			for (uint8_t i=0; i < 2; ++i) {
				destr_cmd_3(c->childs[i]);
				free(c->childs[i]);
				c->childs[i] = NULL;
			}
		break;
		case C_RED:
			if (c->red.c) {
				destr_cmd_3(c->red.c);
				free(c->red.c);
				c->red.c = NULL;
			}
			destr_red_list(&c->red.r);
		break;
		case C_WHL:
			if (c->whl.cnd) {
				destr_cmd_3(c->whl.cnd);
				free(c->whl.cnd);
				c->whl.cnd = NULL;
			}
			if (c->whl.bdy) {
				destr_cmd_3(c->whl.bdy);
				free(c->whl.bdy);
				c->whl.bdy = NULL;
			}
		break;
	}
}

// --Pretty print--

void pp_cmd_0(FILE* f, const cmd_0_t* c) {
	switch (c->ty) {
		case C_BAS:
			fprintf(f, "%s", c->bas);
		break;
		case C_SUB:
			if (c->sub) {
				fwrite("( ", 1, 2, f);
				pp_cmd_3(f, 'c', c->sub);
				fwrite(" ) ", 1, 3, f);
			} else fwrite("() ", 1, 3, f);
		break;
	}
}
void pp_redir(FILE* f, const redir_t* r) {
	fprintf(f, "%d", r->fd);
	switch (r->type) {
		case IN_DUP:     fprintf(f, "<&%d ", r->src_fd); break;
		case OUT_DUP:    fprintf(f, ">&%d ", r->src_fd); break;
		case IN_FILE:    fprintf(f, "<%s ",  r->filewd); break;
		case OUT_TRUNC:  fprintf(f, ">%s ",  r->filewd); break;
		case OUT_APPEND: fprintf(f, ">>%s ", r->filewd); break;
	}
}
void pp_red_list(FILE* f, const red_list_t* r) {
	for (size_t i = 0; i < r->redc; ++i)
		pp_redir(f, r->reds + i);
}
void pp_cmd_1(FILE* f, const cmd_1_t* c) {
	pp_cmd_0(f, &c->c);
	pp_red_list(f, &c->r);
}
void pp_cmd_2(FILE* f, const cmd_2_t* c) {
	for (size_t i = 0; i < c->cmdc; ++i) {
		if (i) fwrite("  |  ", 1, 5, f);
		pp_cmd_1(f, c->cmds + i);
	}
}
void pp_cmd_3(FILE* f, char lvl, const cmd_3_t* c) {
	switch (c->ty) {
		case C_CM2:
			pp_cmd_2(f, &c->cm2);
			break;
		case C_AND:
			if (lvl < 'b') fwrite("{", 1, 1, f);
			pp_cmd_3(f, 'b', c->childs[0]);
			fwrite(" && ", 1, 4, f);
			pp_cmd_3(f, 'a', c->childs[1]);
			if (lvl < 'b') fwrite(";}", 1, 2, f);
			break;
		case C_OR:
			if (lvl < 'b') fwrite("{", 1, 1, f);
			pp_cmd_3(f, 'b', c->childs[0]);
			fwrite(" || ", 1, 4, f);
			pp_cmd_3(f, 'a', c->childs[1]);
			if (lvl < 'b') fwrite(";}", 1, 2, f);
			break;
		case C_SEQ:
			if (lvl < 'c') fwrite("{", 1, 1, f);
			pp_cmd_3(f, 'c', c->childs[0]);
			fwrite(" ; ", 1, 3, f);
			pp_cmd_3(f, 'c', c->childs[1]);
			if (lvl < 'c') fwrite(";}", 1, 2, f);
			break;
		case C_BG:
			if (lvl < 'c') fwrite("{", 1, 1, f);
			pp_cmd_3(f, 'b', c->childs[0]);
			fwrite(" & ", 1, 3, f);
			if (lvl < 'c') fwrite(";}", 1, 2, f);
			break;
		case C_RED:
			fwrite("{", 1, 1, f);
			if (c->red.c) pp_cmd_3(f, 'c', c->red.c);
			fwrite(";}", 1, 2, f);
			pp_red_list(f, &c->red.r);
			break;
		case C_WHL:
			fwrite("while ", 1, 6, f);
			if (c->whl.cnd) pp_cmd_3(f, 'c', c->whl.cnd);
			fwrite(";do ", 1, 4, f);
			if (c->whl.bdy) pp_cmd_3(f, 'c', c->whl.bdy);
			fwrite(";done ", 1, 6, f);
			break;
	}
}
const cmd_3_t* cmd_top(const cmd_3_t* c) {
	while (c->parent) c = c->parent;
	return c;
}
