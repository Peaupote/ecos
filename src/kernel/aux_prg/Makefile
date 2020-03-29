export CC=x86_64-elf-gcc
export AS=x86_64-elf-as
export FLAGS=-O2 -Wall -Wextra -std=gnu99 -nostdlib -mno-red-zone \
	  -fno-builtin -ffreestanding -D__is_libc 
CFILES=init1.c
OUT=$(CFILES:.c=.out)
SRCDEP=$(CFILES) start.S
INCLUDES=-I ../../../include

.PHONY: all clean libc

%.o:%.S

%.s:%.S
	$(CC) -E $< -o $@ $(INCLUDES)

%.o:%.s
	$(AS) $< -o $@ $(INCLUDES)

%.out:%.c libc start.o
	$(CC) $(FLAGS) -o $@ $< start.o ../../libc/libc.a $(INCLUDES) -lgcc

all:$(OUT)

libc:
	$(MAKE) -C ../../libc libc.a

.depends:
	../../../tools/mkdep.sh "$(CC) $(FLAGS) $(INCLUDES)" $(SRCDEP) > .depends

include .depends

clean:
	rm -f *.out *.s *.o
