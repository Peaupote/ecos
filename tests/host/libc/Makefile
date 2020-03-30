include ../vars.Makefile

ROOT=../../..
SRC_LIBC=$(ROOT)/src/libc
ASFILES=
include list.Makefile
INCLUDES=-I $(ROOT)/include

.PHONY: run libc libk

all: run

%.libc.out:%.c libc
	$(CC) $(FLAGS_LC) -o $@ $< $(SRC_LIBC)/$(NAME_LC).a $(INCLUDES)

%.libk.out:%.c libk
	$(CC) $(FLAGS_LK) -o $@ $< $(SRC_LIBC)/$(NAME_LK).a $(INCLUDES)

libc:
	$(MAKE) -C $(SRC_LIBC) $(NAME_LC).a

libk:
	$(MAKE) -C $(SRC_LIBC) $(NAME_LK).a

run: $(TESTS_LIBC)
	@echo $(TESTS_LIBC)
	$(foreach t, $(TESTS_LIBC), @./$(t))

clean:
	rm -f *.o *.out *.s
