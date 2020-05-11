#ifndef _H_LIBC_STDIO
#define _H_LIBC_STDIO

#include <libc/stdio_strfuns.h>

#ifndef __is_kernel

int  putchar(int c);

int  puts(const char *s);

typedef struct io_file {
    int   fd;     // file descriptor to open file
    int   flags;  // remember flags with which file was open

    unsigned char *read_ptr;  // current read pointer
    unsigned char *read_end;  // end data
    unsigned char *read_buf;

    unsigned char *write_ptr; // current write ptr
    unsigned char *write_end; // end data
    unsigned char *write_buf;
} FILE;

FILE *fopen(const char *fname, const char *mode);
FILE *fdopen(int fd, const char *mode);
int   fclose(FILE *stream);

FILE *stdin;
FILE *stdout;
FILE *stderr;

#define EOF (-1)

/**
 * Read nmem items, each size bytes long, from stream to ptr
 * returns number of items readed
 */
size_t fread(void *ptr, size_t size, size_t nmem, FILE *stream);

/**
 * Write nmem items, each size bytes long, from ptr to stream
 * returns number of items readed
 */
size_t fwrite(const void *ptr, size_t size, size_t nmem, FILE *stream);

int fseek(FILE *stream, long offset, int whence);
int fflush(FILE *stream);

int fprintf(FILE *stream, const char *fmt, ...);
int dprintf(int fd, const char *fmt, ...);
int printf(const char *fmt, ...);

int vprintf(const char *format, va_list ap);
int vfprintf(FILE *stream, const char *format, va_list ap);
int vsprintf(char *str, const char *format, va_list ap);

int fgetc(FILE *stream);
char *fgets(char *s, int size, FILE *stream);
/** Replace le caractère c, 1 seul ungetc consécutif est garanti */
int ungetc(int c, FILE* stream);

int fputc(int c, FILE *stream);
int fputs(const char *s, FILE *stream);

ssize_t getline(char **lineptr, size_t *n, FILE *stream);
ssize_t getdelim(char **lineptr, size_t *n, int delim, FILE *stream);

int  fscanf(FILE* stream, const char *format, ...);
int  scanf(const char *format, ...);

void perror(const char *str);

#endif

int sprintf(char *str, const char *format, ...);

int sscanf(const char *str, const char *format, ...);

#endif
