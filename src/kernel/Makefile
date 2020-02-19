CC=x86_64-elf-gcc
AS=x86_64-elf-as
FLAGS=-O2 -Wall -Wextra -std=gnu99 -nostdlib -mcmodel=large -ffreestanding -mno-red-zone
FILES=proc.c file.c tty.c
SRCDEP=main.c kmem.c "-MT entry.s entry.S" tests.c
OBJ=main.o entry.o kmem.o tests.o
UDEP=vga.o string.o
export TRG=64
INCLUDES=-I ../libc/include

.PHONY: all re clean

all: kernel.bin

%.o:%.c
	$(CC) $(FLAGS) -o $@ -c $< $(INCLUDES)

entry.o: entry.s
	$(AS) entry.s -o entry.o

libc: util
	$(MAKE) -C ../libc

UDEP_FROM_HERE=$(patsubst %,../util/64/%,$(UDEP))
UDEP_FROM_UTIL=$(patsubst %,64/%,$(UDEP))

util:
	$(MAKE) -C ../util $(UDEP_FROM_UTIL)

kernel.bin:linker.ld ../boot/boot_data.ld $(OBJ) util
	$(CC) -T linker.ld -o kernel.bin $(FLAGS) $(OBJ) $(UDEP_FROM_HERE) -lgcc

.depends:
	../../tools/mkdep.sh "$(CC) $(FLAGS)" $(SRCDEP) > .depends

include .depends

clean:
	@echo "Clean trash files"
	rm -f *.o *.bin .depends

re: clean $(NAME)
