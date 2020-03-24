LIBC_FILES=string/memcmp.c \
	  string/memcpy.c \
	  string/memset.c \
	  string/memmove.c \
	  string/strcpy.c \
	  string/strncpy.c \
	  string/strlen.c \
	  string/strcmp.c \
	  string/strncmp.c \
	  string/index.c \
	  stdio/printf.c \
	  stdio/putchar.c \
	  stdio/puts.c
LIBC_OBJ=$(LIBC_FILES:%.c=%.libc.o)

LIBK_FILES=string/memcmp.c \
	  string/memcpy.c \
	  string/memset.c \
	  string/memmove.c \
	  string/strcpy.c \
	  string/strncpy.c \
	  string/strlen.c \
	  string/strcmp.c \
	  string/strncmp.c \
	  string/index.c \
	  stdio/printf.c
LIBK_OBJ=$(LIBK_FILES:%.c=%.libk.o)
