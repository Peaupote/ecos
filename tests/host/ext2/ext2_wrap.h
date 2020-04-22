#ifndef _EXT2_WRAP_H
#define _EXT2_WRAP_H

#include<stdint.h>
#include<stddef.h>
#include<stdbool.h>

int init_ext2(void* fs);

bool is_curr_dir();
void print_type(uint16_t type);
int ls();
void print_stat(const char *);
void do_touch(char* s);
void do_cat(char *s);
void do_cd(const char* s);
bool do_mkdir(char* s);
void* save_area(size_t* sz);
void dump();

#endif
