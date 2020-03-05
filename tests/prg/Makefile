export CC=x86_64-elf-gcc
export FLAGS=-O2 -Wall -Wextra -std=gnu99 -nostdlib\
             -ffreestanding -mno-red-zone
OUT=t0.out

.PHONY: all clean

%.out:%.s
	$(CC) $(FLAGS) -o $@ $<

all:$(OUT)

clean:
	rm -f *.out
