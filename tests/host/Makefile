export TEST_SEED?=

include ./vars.Makefile
ifdef TEST_SEED
	FLAGS_AUX:=$(FLAGS_T) -DTEST_SEED=$(TEST_SEED)
else
	FLAGS_AUX=$(FLAGS_T)
endif

INCLUDES=-I ../../include

.PHONY: all clean libc ext2 unit depends

all: libc ext2 unit

%.o:%.c
	@$(CC) $(FLAGS_AUX) -o $@ -c $< $(INCLUDES)

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
	rm -f *.o
