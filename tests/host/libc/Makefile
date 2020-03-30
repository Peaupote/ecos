include ../vars.Makefile

SRC_LIBC=../../../src/libc
ASFILES=
include list.Makefile
INCLUDES=-I ../../include

.PHONY: run libc libk build_libk

all: run

build_libk: $(TESTS_LIBK)

%.libc.out:%.c libc
	$(CC) $(FLAGS_LC) -o $@ $(SRC_LIBC)/$(NAME_LC).a -c $< $(INCLUDES)

%.libk.out:%.c libk
	$(CC) $(FLAGS_LK) -o $@ $(SRC_LIBC)/$(NAME_LK).a -c $< $(INCLUDES)

libc:
	$(MAKE) -C $(SRC_LIBC) libc.a

libk:
	$(MAKE) -C $(SRC_LIBC) libk.a

run: $(TESTS_LIBC)
	@echo $(TESTS_LIBC)
	$(foreach t, $(TESTS_LIBC), @./$(t))

clean:
	rm -f *.o *.out *.s
