export TRG=32
export CC=i686-elf-gcc
export FLAGS=-O2 -g -Wall -Wextra -std=gnu99 -nostdlib -ffreestanding
AS=i686-elf-as
SRCDEP="-MT boot.s boot.S" init.c load.c
UDEP=vga.o string.o elf64.o
OBJ=boot.o init.o load.o

.PHONY: clean all util

all: boot.o init.o

indj_64.s: indj_64.S
	x86_64-elf-as indj_64.S -o indj_64.o
	x86_64-elf-objcopy indj_64.o -O binary
	xxd -ps indj_64.o | gawk \
		'BEGIN{printf(".byte ")}\
		{for(i=0;i<length($$0);i+=2){\
			if(i!=0) printf(",");\
			printf("0x%s", substr($$0,i+1,2))\
		}}\
		END{printf("\n")}' > indj_64.s
	rm -f indj_64.o

gdt.s: gdt.txt
	cpp -P gdt.txt | gawk \
		'BEGIN{ofs=0;}\
		{\
		printf("movl $$GDT_ENTRY_0(%s, %s), (gd_table + %d)\n", \
			$$1, $$2, ofs);\
		ofs += 4;\
		printf("movl $$GDT_ENTRY_1(%s, %s, %s, %s), (gd_table + %d)\n", \
			$$1, $$2, $$3, $$4, ofs);\
		ofs += 4;}' > gdt.s

boot.o: boot.s
	$(AS) boot.s -o boot.o

%.o:%.c
	$(CC) $(FLAGS) -o $@ -c $<

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
