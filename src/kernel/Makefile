CC=x86_64-elf-gcc
AS=x86_64-elf-as
FLAGS=-O2 -Wall -Wextra -std=gnu99 -nostdlib -mcmodel=large -ffreestanding -mno-red-zone
FILES=proc.c file.c tty.c
OBJ=main.o entry.o kmem.o tests.o
UDEP=vga.o string.o
export TRG=64
INCLUDES=-I ../libc/include

.PHONY: all re clean

all: kernel.bin

%.o:%.c
	$(CC) $(FLAGS) -o $@ -c $< $(INCLUDES)

main.o: main.c ../util/vga.h ../util/string.h kmem.h

tests.o: tests.c tests.h ../util/vga.h ../util/string.h kmem.h

kmem.o: kmem.c kmem.h ../util/paging.h

entry.o: entry.s
	$(AS) entry.s -o entry.o

libc: util
	$(MAKE) -C ../libc

util:
	$(MAKE) -C ../util $(patsubst %,64/%,$(UDEP))

kernel.bin: linker.ld ../boot/boot_data.ld $(OBJ) util
	$(CC) -T linker.ld -o kernel.bin $(FLAGS) $(OBJ) \
		$(patsubst %,../util/64/%,$(UDEP)) \
		-lgcc

clean:
	@echo "Clean trash files"
	rm -f *.o *.bin

re: clean $(NAME)
