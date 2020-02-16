# Default Target:
TRG?=64
FLAGS=-O2 -g -Wall -Wextra -std=gnu99 -nostdlib -ffreestanding -mno-red-zone
ifeq ($(TRG),64)
	CC=x86_64-elf-gcc
else ifeq ($(TRG),test)
	CC=gcc
	FLAGS=-O2 -g -Wall -Wextra -std=gnu99
else
	CC=i686-elf-gcc
endif

$(TRG):
	mkdir -p $(TRG)

$(TRG)/%.o:%.c $(TRG)
	$(CC) $(FLAGS) -o $@ -c $<

$(TRG)/vga.o: vga.c vga.h string.h
$(TRG)/string.o: string.c string.h
$(TRG)/elf64.o: elf64.c elf64.h

.PHONY: clean

clean:
	rm -rf 32 64 test
