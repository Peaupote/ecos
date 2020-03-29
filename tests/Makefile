export TRG=host
export CC=gcc
export FLAGS=-O2 -Wall -Wextra -std=gnu99 -fno-builtin

.PHONY: all clean libc ext2 unit depends

all: libc ext2 unit

unit:
	$(MAKE) -C unit run

ext2:
	$(MAKE) -C ext2 all

libc:
	$(MAKE) -C libc run

depends:
	$(MAKE) -B -C unit .depends

clean:
	rm -f *.out *.o
	$(MAKE) -C prg  clean
	$(MAKE) -C unit clean
	$(MAKE) -C ext2 clean
	$(MAKE) -C libc clean
