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
SRCDEP=vga.c string.c elf64.c

$(TRG)/%.o:%.c
	mkdir -p $(TRG)
	$(CC) $(FLAGS) -o $@ -c $<

$(TRG)/.depends:
	mkdir -p $(TRG)
	../../tools/mkdep.sh "$(CC) $(FLAGS)"\
		$(foreach file,$(SRCDEP),"-MT 32/$(file) $(file)")\
		> $(TRG)/.depends

include $(TRG)/.depends

.PHONY: clean

clean:
	rm -rf 32 64 test
