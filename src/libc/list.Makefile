LIBC_FILES=string/memcmp.c \
	  string/memcpy.c \
	  string/memset.c \
	  string/memmove.c \
	  string/strcpy.c \
	  string/strlen.c \
	  stdio/printf.c \
	  stdio/putchar.c \
	  stdio/puts.c
LIBC_OBJ=$(LIBC_FILES:%.c=%.o)