AS=i686-elf-as
CC=i686-elf-gcc
FLAGS=-O2 -g -Wall -Wextra -std=gnu99 -nostdlib -ffreestanding -mno-red-zone

.PHONY: clean all

all: boot.o init.o

boot.o: boot.s
	$(AS) boot.s -o boot.o

%.o:%.c
	$(CC) $(FLAGS) -c $? -o $@

clean:
	rm -f *.o
