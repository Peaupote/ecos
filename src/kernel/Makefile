export TRG=64
export CC=x86_64-elf-gcc
export AS=x86_64-elf-as
export AR=x86_64-elf-ar
include ../../vars.Makefile
export FLAGS_U=$(FLAGS_K)

include list.Makefile
KBMAP=gawk -f ../../tools/keyboard_map.awk
UDEP=vga.o string.o elf64.o
INCLUDES=-I ../../include

.PHONY: all re clean util libk tests_prg home_part ext2

all: kernel.bin

%.o:%.c
	$(CC) $(FLAGS_K) -o $@ -c $< $(INCLUDES)

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

data.o:data.s tests_home

UDEP_FROM_HERE=$(patsubst %,../util/64/%,$(UDEP))
UDEP_FROM_UTIL=$(patsubst %,64/%,$(UDEP))

libk:
	$(MAKE) -C ../libc libk.a

util:
	$(MAKE) -C ../util $(UDEP_FROM_UTIL)

tests_home:
	$(MAKE) -C ../../tests/run home.img

ext2:
	$(MAKE) -C ../fs/ext2 ext2.a

kernel.bin:linker.ld ../boot/boot_data.ld $(OBJ) util libk ext2
	$(CC) -T linker.ld -o kernel.bin $(FLAGS_K) $(LIBK_OBJ) \
		$(OBJ) $(UDEP_FROM_HERE) ../libc/libk.a \
		../fs/ext2/ext2.a $(INCLUDES) -lgcc

.depends:
	../../tools/mkdep.sh "$(CC) $(FLAGS_K) $(INCLUDES)" $(SRCDEP) > .depends

include .depends

clean:
	@echo "Clean trash files"
	rm -f *.o *.bin .depends *.s */*.s $(OBJ)

re: clean $(NAME)
