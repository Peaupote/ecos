export TRG=test
export CC=gcc
export FLAGS=-O2 -Wall -Wextra -std=gnu99 -fno-builtin
SRCDEP=elf_info.c
UDEP=elf64.o string.o
INCLUDES=-I ../include

.PHONY: all util clean libc ext2

all: elf_info.out util libc ext2

%.o:%.c
	$(CC) $(FLAGS) -o $@ -c $< $(INCLUDES)

UDEP_FROM_HERE=$(patsubst %,../src/util/test/%,$(UDEP))
UDEP_FROM_UTIL=$(patsubst %,test/%,$(UDEP))

util:
	$(MAKE) -C ../src/util $(UDEP_FROM_UTIL)

ext2:
	$(MAKE) -C ./ext2 all

libc:
	$(MAKE) -C libc run

elf_info.out:elf_info.o util
	$(CC) $(FLAGS) -o elf_info.out elf_info.o $(UDEP_FROM_HERE) $(INCLUDES)

.depends:
	../tools/mkdep.sh "$(CC) $(FLAGS)" $(SRCDEP) > .depends

include .depends

clean:
	rm -f *.out *.o .depends
	$(MAKE) -C prg  clean
	$(MAKE) -C unit clean
	$(MAKE) -C ext2 clean
	$(MAKE) -C libc clean
