CC=x86_64-elf-gcc
AS=x86_64-elf-as
FLAGS=-O2 -g -Wall -Wextra -std=gnu99 -nostdlib -ffreestanding -mno-red-zone
FILES=proc.c file.c tty.c
OBJ=main.o entry.o
UDEP=vga.o string.o
export TRG=64
INCLUDES=-I ../libc/include

.PHONY: all re clean

all: kernel.bin

%.o:%.c
	$(CC) $(FLAGS) -o $@ -c $< $(INCLUDES)

main.o: main.c ../util/vga.h

entry.o: entry.s
	$(AS) entry.s -o entry.o

libc: util
	$(MAKE) -C ../libc

util:
	$(MAKE) -C ../util $(patsubst %,64/%,$(UDEP))

kernel.bin: linker.ld $(OBJ) libc util
	$(CC) -T linker.ld -o kernel.bin $(FLAGS) $(OBJ) \
		$(patsubst %,../util/64/%,$(UDEP)) \
		../libc/*/*.o -lgcc # un peu degeux la

clean:
	@echo "Clean trash files"
	rm -f *.o *.bin

re: clean $(NAME)
