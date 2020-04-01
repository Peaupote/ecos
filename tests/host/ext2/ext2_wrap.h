#ifndef _EXT2_WRAP_H
#define _EXT2_WRAP_H

#include<stdint.h>
#include<stddef.h>

int init_ext2(void* fs);

void print_type(uint8_t type);
int ls();
void print_stat();
void do_cd(const char* s);
void do_mkdir(char* s);
void* save_area(size_t* sz);
void dump();

#endif
