export CC=x86_64-elf-gcc
export FLAGS=-O2 -Wall -Wextra -std=gnu99 -nostdlib\
             -ffreestanding -mno-red-zone
INCLUDES=-I ../../include
OUT=t0.out args.out

.PHONY: all clean

%.s:%.S
	$(CC) -E $< -o $@ $(INCLUDES)

%.out:%.s
	$(CC) $(FLAGS) -o $@ $< $(INCLUDES)

all:$(OUT)

clean:
	rm -f *.out
