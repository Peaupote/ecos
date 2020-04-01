export TRG=host
export CC=gcc
export FLAGS_T=-O2 -Wall -Wextra -std=gnu99 -fno-builtin -D__use_host_libc
export FLAGS_U=$(FLAGS_T)

SRCDEP=elf_info.c
UDEP=elf64.o string.o
INCLUDES=-I ../include

.PHONY: all util clean

all: elf_info.out

%.o:%.c
	$(CC) $(FLAGS_T) -o $@ -c $< $(INCLUDES)

UDEP_FROM_HERE=$(patsubst %,../src/util/host/%,$(UDEP))
UDEP_FROM_UTIL=$(patsubst %,host/%,$(UDEP))

util:
	$(MAKE) -C ../src/util $(UDEP_FROM_UTIL)

elf_info.out:elf_info.o util
	$(CC) -g $(FLAGS_T) -o $@ elf_info.o $(UDEP_FROM_HERE) $(INCLUDES)

.depends:
	../tools/mkdep.sh "$(CC) $(FLAGS_T) $(INCLUDES)" $(SRCDEP) > .depends

include .depends

clean:
	rm -f *.out *.o .depends
