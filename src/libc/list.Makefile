LIBC_AS_FILES?=sys.S signal/signal_as.S
LIBC_CFILES?=libc_main.c \
	  string/memcmp.c \
	  string/memcpy.c \
	  string/memset.c \
	  string/memmove.c \
	  string/strcpy.c \
	  string/strncpy.c \
	  string/strlen.c \
	  string/strcmp.c \
	  string/strncmp.c \
	  string/index.c \
	  string/strtok_r.c \
	  string/strtok.c \
	  string/strchr.c \
	  string/strrchr.c \
	  string/strchrnul.c \
	  stdio/printf.c \
	  stdio/putchar.c \
	  stdio/puts.c \
	  stdio/perror.c \
	  stdio/file.c \
	  stdlib/atoi.c \
	  stdlib/malloc.c \
	  stdlib/env.c \
	  unistd/brk.c \
	  unistd/exec.c \
	  stdio/scanf.c \
	  ctype/ctype.c \
	  sys/ressource.c \
	  sys/stat.c \
	  dirent/opendir.c \
	  dirent/readdir.c \
	  signal/signal.c \
	  assert/assert.c
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
	  string/strchr.c \
	  string/strrchr.c \
	  string/strchrnul.c \
	  string/strtok_r.c \
	  stdio/printf.c \
	  stdio/scanf.c \
	  ctype/ctype.c
LIBK_DEP=$(foreach file,$(LIBK_CFILES),\
			"-MT $(file:.c=.$(NAME_LK).o) $(file)")
LIBK_OBJ=$(LIBK_CFILES:%.c=%.$(NAME_LK).o)\
			$(LIBK_AS_FILES:%.S=%.$(NAME_LK).o)
