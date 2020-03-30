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
	$(MAKE) -C unit clean
	$(MAKE) -C ext2 clean
	$(MAKE) -C libc clean
