FLAGS=-O1 -Wall -Wextra -std=gnu99
CC=gcc
ASFILES=
include ../../src/libc/list.Makefile
LIBC_FILES:=$(addprefix ../../src/libc/, $(LIBC_FILES))
TEST_FILES=string.c
OBJ=$(LIBC_FILES:.c=.o)
TESTS=$(TEST_FILES:.c=.out)
INCLUDES=-I ../../include

.PHONY: run libc

all: run

%.out:%.c libc
	$(CC) $(FLAGS) -o $@ ../../src/libc/libc.a $< $(INCLUDES)

libc:
	$(MAKE) -C ../../src/libc

run: libc $(OBJ) $(TESTS)
	$(foreach t, $(TESTS), ./$(t))

clean:
	rm -rf *.o *.out *.s
