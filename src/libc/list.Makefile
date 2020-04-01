LIBC_AS_FILES?=sys.S
LIBC_CFILES?=string/memcmp.c \
	  string/memcpy.c \
	  string/memset.c \
	  string/memmove.c \
	  string/strcpy.c \
	  string/strncpy.c \
	  string/strlen.c \
	  string/strcmp.c \
	  string/strncmp.c \
	  string/index.c \
	  string/strtok.c \
	  string/strchr.c \
	  stdio/printf.c \
	  stdio/putchar.c \
	  stdio/puts.c \
	  stdlib/malloc.c \
	  unistd/brk.c
LIBC_DEP=$(foreach file,$(LIBC_CFILES),\
			"-MT $(file:.c=.$(NAME_LC).o) $(file)")
LIBC_OBJ=$(LIBC_CFILES:%.c=%.$(NAME_LC).o)\
			$(LIBC_AS_FILES:%.S=%.$(NAME_LC).o)

LIBK_AS_FILES?=sys.S
LIBK_CFILES?=string/memcmp.c \
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
LIBK_DEP=$(foreach file,$(LIBK_CFILES),\
			"-MT $(file:.c=.$(NAME_LK).o) $(file)")
LIBK_OBJ=$(LIBK_CFILES:%.c=%.$(NAME_LK).o)\
			$(LIBK_AS_FILES:%.S=%.$(NAME_LK).o)
