export TRG=32
export CC=i686-elf-gcc
export FLAGS=-O2 -g -Wall -Wextra -std=gnu99 -nostdlib -ffreestanding
AS=i686-elf-as
CFILES=init.c load.c
ASFILES=boot.S
SRCDEP=$(foreach file,$(ASFILES),"-MT $(file:.S=.s) $(file)") $(CFILES)
OBJ=$(ASFILES:.S=.o) $(CFILES:.c=.o)
UDEP=vga.o string.o elf64.o
INCLUDES=-I ../../include

.PHONY: clean all util

all: boot.bin

gdt.s: gdt.txt
	cpp -P gdt.txt | gawk -f ../../tools/fillGDT.awk > gdt.s

%.o:%.c
	$(CC) $(FLAGS) -o $@ -c $< $(INCLUDES)

%.s:%.S
	$(CC) -E $< -o $@ $(INCLUDES)

%.o:%.s
	$(AS) $< -o $@ $(INCLUDES)

UDEP_FROM_HERE=$(patsubst %,../util/32/%,$(UDEP))
UDEP_FROM_UTIL=$(patsubst %,32/%,$(UDEP))

util:
	$(MAKE) -C ../util $(UDEP_FROM_UTIL)

boot.bin: linker.ld boot_data.ld $(OBJ) util
	$(CC) -T linker.ld -o boot.bin $(FLAGS) $(OBJ) $(UDEP_FROM_HERE) -lgcc
	@echo "Grub check multiboot"
	grub-file --is-x86-multiboot boot.bin

.depends:
	../../tools/mkdep.sh "$(CC) $(FLAGS)" $(SRCDEP) > .depends

include .depends

clean:
	rm -f *.o *.bin *.s boot.map .depends
