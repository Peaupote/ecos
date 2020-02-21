export TRG=64
export CC=x86_64-elf-gcc
export INCLUDES=-I ../libc/include
export FLAGS=-O2 -Wall -Wextra -std=gnu99 -nostdlib -mcmodel=large -ffreestanding -mno-red-zone
AS=x86_64-elf-as
include ../libc/list.Makefile
FILES=proc.c file.c tty.c idt.c
SRCDEP=main.c kmem.c "-MT entry.s entry.S" idt.c tests.c
OBJ=main.o entry.o kmem.o idt.o tests.o
UDEP=vga.o string.o

.PHONY: all re clean

all: kernel.bin

%.o:%.c
	$(CC) $(FLAGS) -o $@ -c $< $(INCLUDES)

irq_handlers.s: mkIRQH.awk
	seq 0 31 | gawk -f mkIRQH.awk > irq_handlers.s

entry.o: entry.s
	$(AS) entry.s -o entry.o

libc: util
	$(MAKE) -C ../libc all

UDEP_FROM_HERE=$(patsubst %,../util/64/%,$(UDEP))
UDEP_FROM_UTIL=$(patsubst %,64/%,$(UDEP))

util:
	$(MAKE) -C ../util $(UDEP_FROM_UTIL)

kernel.bin:linker.ld ../boot/boot_data.ld $(OBJ) util libc
	$(CC) -T linker.ld -o kernel.bin $(FLAGS) $(OBJ) $(UDEP_FROM_HERE) \
		$(patsubst %,../libc/%,$(LIBC_OBJ)) \
		$(INCLUDES) -lgcc

.depends:
	../../tools/mkdep.sh "$(CC) $(FLAGS) $(INCLUDES)" $(SRCDEP) > .depends

include .depends

clean:
	@echo "Clean trash files"
	rm -f *.o *.bin .depends entry.s irq_handlers.s

re: clean $(NAME)
