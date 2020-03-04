export TRG=64
export CC=x86_64-elf-gcc
export INCLUDES=-I ../libc/include
export FLAGS=-O2 -Wall -Wextra -std=gnu99 -nostdlib \
			 -mcmodel=large -ffreestanding -mno-red-zone
AS=x86_64-elf-as
include ../libc/list.Makefile
CFILES=main.c kmem.c idt.c gdt.c keyboard.c tty.c tests.c \
		syscalls/sleep.c syscalls/sys.c proc.c
ASFILES=entry.S idt_as.S data.S sys.S
SRCDEP=$(foreach file,$(ASFILES),"-MT $(file:.S=.s) $(file)") $(CFILES)
OBJ=$(ASFILES:.S=.o) $(CFILES:.c=.o)
KBMAP=gawk -f ../../tools/keyboard_map.awk
UDEP=vga.o string.o

.PHONY: all re clean

all: kernel.bin

%.o:%.c
	$(CC) $(FLAGS) -o $@ -c $< $(INCLUDES)

%.o:%.s
	$(AS) $< -o $@

int_handlers_tab.s:
	seq 0 31 | gawk '{printf(".quad irq%d\n",$$1)}' > int_handlers_tab.s
int_handlers.s: int_handlers.txt int_handlers.awk
	cpp -P int_handlers.txt | gawk -f int_handlers.awk > int_handlers.s

keyboard_layout.s: ../../data/keyboard_azerty.txt ../../data/keyboard_qwerty.txt
	echo "keyboard_azerty_layout:" > keyboard_layout.s
	$(KBMAP) ../../data/keyboard_azerty.txt >> keyboard_layout.s
	echo "keyboard_qwerty_layout:" >> keyboard_layout.s
	$(KBMAP) ../../data/keyboard_qwerty.txt >> keyboard_layout.s

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
	rm -f *.o *.bin .depends *.s

re: clean $(NAME)
