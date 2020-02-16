CC=x86_64-elf-gcc
AS=x86_64-elf-as
FLAGS=-O2 -g -Wall -Wextra -std=gnu99 -nostdlib -ffreestanding -mno-red-zone
FILES=proc.c file.c
OBJ=main.o entry.o
UDEP=vga.o string.o
export TRG=64

.PHONY: all re clean

all: kernel.bin

%.o:%.c
	$(CC) $(FLAGS) -o $@ -c $<

main.o: main.c ../util/vga.h

entry.o: entry.s
	$(AS) entry.s -o entry.o

util:
	$(MAKE) -C ../util $(patsubst %,64/%,$(UDEP))

kernel.bin: linker.ld $(OBJ) util
	$(CC) -T linker.ld -o kernel.bin $(FLAGS) $(OBJ) \
		$(patsubst %,../util/64/%,$(UDEP)) -lgcc

clean:
	@echo "Clean trash files"
	rm -f *.o *.bin

re: clean $(NAME)
