#ifndef _H_FILE
#define _H_FILE

typedef int ino_t;

#define FREAD  1
#define FWRITE 2
#define FPIPE  4

struct file {
    ino_t f_inode;    // inode number
    int   f_count;    // reference count
    int   f_mode;     // access mode of file
    char  *f_pointer; // pointer into the file
};

#endif
