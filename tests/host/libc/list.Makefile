TESTS_FILES=string.c scanf.c
TESTS_LIBC=$(TESTS_FILES:%.c=%.libc.out)
TESTS_LIBK=$(TESTS_FILES:%.c=%.libk.out)

export LIBC_AS_FILES=
export LIBC_CFILES=\
	string/strlen.c \
	string/memcmp.c \
	string/strncmp.c \
	stdio/scanf.c \
	ctype/ctype.c
export LIBK_AS_FILES=
export LIBK_CFILES=\
	string/strlen.c \
	string/memcmp.c \
	string/strncmp.c
