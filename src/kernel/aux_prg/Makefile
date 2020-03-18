export CC=x86_64-elf-gcc
export FLAGS=-O2 -Wall -Wextra -std=gnu99 -nostdlib\
			 -ffreestanding -mno-red-zone
OUT=init.out
INCLUDES=-I ../../../include

.PHONY: all clean

%.s:%.S
	$(CC) -E $< -o $@ $(INCLUDES)

%.out:%.s
	$(CC) $(FLAGS) -o $@ $<

all:$(OUT)

clean:
	rm -f *.out *.s
