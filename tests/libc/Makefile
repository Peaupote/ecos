LIBC_FLAGS=-O1 -Wall -Wextra -std=gnu99
LIBK_FLAGS?=
CC?=gcc
ASFILES=
include list.Makefile
include ../../src/libc/list.Makefile
LIBC_FILES:=$(addprefix ../../src/libc/, $(LIBC_FILES))
INCLUDES=-I ../../include

.PHONY: run libc libk build_libk

all: run

build_libk: $(TESTS_LIBK)

%.libc.out:%.c libc
	$(CC) $(FLAGS) -D__is_libc -o $@ ../../src/libc/libc.a $< $(INCLUDES)

%.libk.out:%.c libk
	$(CC) $(FLAGS) -D__is_libk -o $@ -c $< $(INCLUDES)

libc:
	$(MAKE) -C ../../src/libc libc.a

libk:
	$(MAKE) -C ../../src/libc libk.a

run: $(TESTS_LIBC)
	@echo $(TESTS_LIBC)
	$(foreach t, $(TESTS_LIBC), @./$(t))

clean:
	rm -rf *.o *.out *.s
