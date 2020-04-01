include ../vars.Makefile

ROOT=../../..
SRC_LIBC=$(ROOT)/src/libc
ASFILES=
include list.Makefile
INCLUDES=-I $(ROOT)/include

.PHONY: run libc libk

all: run

%.libc.out:%.c libc tutil
	@$(CC) $(FLAGS_T) -o $@ $< ../tutil.o $(INCLUDES) -L. -lct

%.libk.out:%.c libk
	$(CC) $(FLAGS_LK) -o $@ $< $(SRC_LIBC)/$(NAME_LK).a $(INCLUDES)

libc:
	@$(MAKE) -C $(SRC_LIBC) $(NAME_LC).a
	@objcopy $(SRC_LIBC)/$(NAME_LC).a ./libct.a `gawk -f import.awk import-c.h`

libk:
	@$(MAKE) -C $(SRC_LIBC) $(NAME_LK).a
	@objcopy $(SRC_LIBC)/$(NAME_LK).a ./libct.a

tutil:
	@$(MAKE) -C ../ tutil.o

run: $(TESTS_LIBC)
	$(foreach t, $(TESTS_LIBC), "./$(t)" ;)

clean:
	rm -f *.o *.out *.s *.a
