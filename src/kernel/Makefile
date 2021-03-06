export TRG=64
export CC=x86_64-elf-gcc
export AS=x86_64-elf-as
export AR=x86_64-elf-ar
ROOT=../..
include $(ROOT)/vars.Makefile
export FLAGS_U=$(FLAGS_K)

include list.Makefile
KBMAP=gawk -f ../../tools/keyboard_map.awk
UDEP=elf64.o celf.o
INCLUDES=-I ../../include

.PHONY: all re clean util libk tests_prg home_part ext2

all: kernel.bin

%.o:%.c
	$(CC) $(FLAGS_K) -o $@ -c $< $(INCLUDES)

%.s:%.S
	$(CC) -E $< -o $@ $(INCLUDES)

%.o:%.s
	$(AS) $< -o $@ $(INCLUDES)

keyboard_layout.s: ../../data/keyboard_azerty.txt ../../data/keyboard_qwerty.txt
	echo "keyboard_azerty_layout:" > keyboard_layout.s
	$(KBMAP) ../../data/keyboard_azerty.txt >> keyboard_layout.s
	echo "keyboard_qwerty_layout:" >> keyboard_layout.s
	$(KBMAP) ../../data/keyboard_qwerty.txt >> keyboard_layout.s

data.o:data.s libc root_img test_img $(ROOT)/data/font.data

UDEP_FROM_HERE=$(patsubst %,../util/64/%,$(UDEP))
UDEP_FROM_UTIL=$(patsubst %,64/%,$(UDEP))

libk:
	$(MAKE) -C ../libc libk.a

libc:
	$(MAKE) -C ../libc libc.out

util:
	$(MAKE) -C ../util $(UDEP_FROM_UTIL)

root_img:
	$(MAKE) -C ../sys root.img

test_img:
	$(MAKE) -C $(ROOT)/tests/run home.img

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
