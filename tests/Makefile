export TRG=test
export CC=gcc
export FLAGS=-O2 -Wall -Wextra -std=gnu99
SRCDEP=elf_info.c
UDEP=elf64.o string.o

.PHONY: all util clean

all: elf_info.out

%.o:%.c
	$(CC) $(FLAGS) -o $@ -c $<

UDEP_FROM_HERE=$(patsubst %,../src/util/test/%,$(UDEP))
UDEP_FROM_UTIL=$(patsubst %,test/%,$(UDEP))

util:
	$(MAKE) -C ../src/util $(UDEP_FROM_UTIL)

elf_info.out:elf_info.o util
	$(CC) $(FLAGS) -o elf_info.out elf_info.o $(UDEP_FROM_HERE)

.depends:
	../tools/mkdep.sh "$(CC) $(FLAGS)" $(SRCDEP) > .depends

include .depends

clean:
	rm -f *.out *.o .depends
	$(MAKE) -C prg clean
	$(MAKE) -C unit clean
