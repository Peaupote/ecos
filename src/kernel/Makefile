export TRG=64
export CC=x86_64-elf-gcc
export FLAGS=-O1 -Wall -Wextra -std=gnu99 -nostdlib \
			 -mcmodel=large -ffreestanding -mno-red-zone
AS=x86_64-elf-as
CFILES=main.c memory/kmem.c idt.c gdt.c keyboard.c tty.c tests.c \
		syscalls/sleep.c syscalls/sys.c proc.c kutil.c \
		memory/page_alloc.c memory/shared_pages.c memory/shared_ptr.c \
		../fs/dummy/dummy.c ../fs/proc/proc.c file.c
ASFILES=entry.S idt_as.S data.S
SRCDEP=$(foreach file,$(ASFILES),"-MT $(file:.S=.s) $(file)") $(CFILES)
OBJ=$(ASFILES:.S=.o) $(CFILES:.c=.o)
KBMAP=gawk -f ../../tools/keyboard_map.awk
UDEP=vga.o string.o elf64.o
INCLUDES=-I ../../include

.PHONY: all re clean util tests_prg aux_prg

all: kernel.bin

%.o:%.c
	$(CC) $(FLAGS) -o $@ -c $< $(INCLUDES)

%.s:%.S
	$(CC) -E $< -o $@ $(INCLUDES)

%.o:%.s
	$(AS) $< -o $@ $(INCLUDES)

int_handlers_tab.s:
	seq 0 31 | gawk '{printf(".quad irq%d\n",$$1)}' > int_handlers_tab.s
int_handlers.s: int_handlers.txt int_handlers.awk
	cpp -P int_handlers.txt | gawk -f int_handlers.awk > int_handlers.s

keyboard_layout.s: ../../data/keyboard_azerty.txt ../../data/keyboard_qwerty.txt
	echo "keyboard_azerty_layout:" > keyboard_layout.s
	$(KBMAP) ../../data/keyboard_azerty.txt >> keyboard_layout.s
	echo "keyboard_qwerty_layout:" >> keyboard_layout.s
	$(KBMAP) ../../data/keyboard_qwerty.txt >> keyboard_layout.s

data.o:data.s tests_prg aux_prg

UDEP_FROM_HERE=$(patsubst %,../util/64/%,$(UDEP))
UDEP_FROM_UTIL=$(patsubst %,64/%,$(UDEP))

util:
	$(MAKE) -C ../util $(UDEP_FROM_UTIL)

aux_prg:
	$(MAKE) -C aux_prg

tests_prg:
	$(MAKE) -C ../../tests/prg all

kernel.bin:linker.ld ../boot/boot_data.ld $(OBJ) util
	$(CC) -T linker.ld -o kernel.bin $(FLAGS) $(OBJ) $(UDEP_FROM_HERE) \
		$(INCLUDES) -lgcc

.depends:
	../../tools/mkdep.sh "$(CC) $(FLAGS) $(INCLUDES)" $(SRCDEP) > .depends

include .depends

clean:
	@echo "Clean trash files"
	$(MAKE) -C aux_prg clean
	rm -f *.o *.bin .depends *.s $(OBJ)

re: clean $(NAME)
