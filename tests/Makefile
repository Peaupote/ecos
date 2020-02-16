CC=gcc
FLAGS=-O2 -Wall -Wextra -std=gnu99
UDEP=elf64.o string.o
export TRG=test

.PHONY: all util clean

all: elf_info.out

util:
	$(MAKE) -C ../src/util $(patsubst %,test/%,$(UDEP))

elf_info.out:elf_info.c ../src/util/elf64.h ../src/util/string.h util
	$(CC) $(FLAGS) -o elf_info.out elf_info.c \
		$(patsubst %,../src/util/test/%,$(UDEP))

clean:
	rm -f *.out *.o
