CC?=gcc
AR?=ar
FLAGS?=-O2 -g -Wall -Wextra -std=gnu99 \
		-nostdlib -mcmodel=large -ffreestanding \
		-mno-red-zone
include list.Makefile
SRCDEP=$(LIBC_FILES) $(LIBK_FILES)
INCLUDES=-I ../../include

##

.PHONY: all re clean

all: libc.a libk.a

%.libc.o:%.c
	$(CC) $(FLAGS) -D__is_libc -c $? -o $@ $(INCLUDES)

%.libk.o:%.c
	$(CC) $(FLAGS) -D__is_libk -c $? -o $@ $(INCLUDES)

libc.a: $(LIBC_OBJ)
	$(AR) rcs $@ $(LIBC_OBJ)

libk.a: $(LIBK_OBJ)
	$(AR) rcs $@ $(LIBK_OBJ)

.depends:
	../../tools/mkdep.sh "$(CC) $(FLAGS)" $(SRCDEP) > .depends

include .depends

clean:
	@echo "Clean trash files"
	rm -rf $(NAME) $(OBJ) *.o **/*.o *.a

re: clean $(NAME)
