LIBC_AS_FILES=sys.s
LIBC_CFILES=string/memcmp.c \
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
LIBC_FILES=$(LIBC_CFILES) $(LIBC_AS_FILES)
LIBC_OBJ=$(LIBC_FILES:%.c=%.libc.o) $(LIBC_AS_FILES:%.s=%.libc.o)

LIBK_AS_FILES=sys.s
LIBK_CFILES=string/memcmp.c \
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
LIBK_FILES=$(LIBK_CFILES) $(LIBK_AS_FILES)
LIBK_OBJ=$(LIBK_FILES:%.c=%.libk.o) $(LIBK_AS_FILES:%.s=%.libk.o)
