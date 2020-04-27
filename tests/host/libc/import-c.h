void* my_stdin;
void* my_stdout;
void* my_stderr;

extern int    my_memcmp(const void*, const void*, size_t);
extern size_t my_strlen(const char* s);
extern int    my_sscanf(const char *str, const char *fmt, ...);
extern int    my_strcmp(const char *lhs, const char *rhs);
extern int    my_strncmp(const char *lhs, const char *rhs, size_t len);
extern char*  my_strtok_rnull(char *str, const char *delim, char** saveptr);
extern int    my_printf(const char *fmt, ...);
