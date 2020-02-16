AS=i686-elf-as
CC=i686-elf-gcc
FLAGS=-O2 -g -Wall -Wextra -std=gnu99 -nostdlib -ffreestanding -mno-red-zone
HDEP=../def.h
UDEP=vga.o string.o elf64.o
OBJ=boot.o init.o load.o
export TRG=32

.PHONY: clean all util

all: boot.o init.o

boot.o: boot.s $(HDEP) def.h
	$(AS) boot.s -o boot.o

%.o:%.c
	$(CC) $(FLAGS) -o $@ -c $<

init.o: init.c $(HDEP)

load.o: load.c multiboot.h ../util/vga.h ../util/string.h ../util/elf64.h

util:
	$(MAKE) -C ../util $(patsubst %,32/%,$(UDEP))

boot.bin: linker.ld $(OBJ) util
	$(CC) -T linker.ld -o boot.bin $(FLAGS) $(OBJ) \
		$(patsubst %,../util/32/%,$(UDEP)) -lgcc
	@echo "Grub check multiboot"
	grub-file --is-x86-multiboot boot.bin

clean:
	rm -f *.o *.bin
