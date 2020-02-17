AS=i686-elf-as
CC=i686-elf-gcc
FLAGS=-O2 -g -Wall -Wextra -std=gnu99 -nostdlib -ffreestanding -mno-red-zone
HDEP=../def.h
UDEP=vga.o string.o elf64.o
OBJ=boot.o init.o load.o
export TRG=32

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

gdt.s: gdt.txt
	cpp -P gdt.txt | gawk \
		'BEGIN{printf("#include \"def.h\"\n"); ofs=0;}\
		{\
		printf("movl $$GDT_ENTRY_0(%s, %s), (gd_table + %d)\n", \
			$$1, $$2, ofs);\
		ofs += 4;\
		printf("movl $$GDT_ENTRY_1(%s, %s, %s, %s), (gd_table + %d)\n", \
			$$1, $$2, $$3, $$4, ofs);\
		ofs += 4;}' > gdt.s

boot.s: boot.S indj_64.s $(HDEP) gdt.s def.h

boot.o: boot.s
	$(AS) boot.s -o boot.o

%.o:%.c
	$(CC) $(FLAGS) -o $@ -c $<

init.o: init.c $(HDEP)

load.o: load.c multiboot.h ../util/vga.h ../util/string.h \
	../util/elf64.h $(HDEP)

util:
	$(MAKE) -C ../util $(patsubst %,32/%,$(UDEP))

boot.bin: linker.ld boot_data.ld $(OBJ) util
	$(CC) -T linker.ld -o boot.bin $(FLAGS) $(OBJ) \
		$(patsubst %,../util/32/%,$(UDEP)) -lgcc
	@echo "Grub check multiboot"
	grub-file --is-x86-multiboot boot.bin

clean:
	rm -f *.o *.bin *.s
