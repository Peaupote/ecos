CC=gcc
AR=ar
FLAGS=-O2 -g -Wall -Wextra -std=gnu99 \
		-nostdlib -mcmodel=large -ffreestanding \
		-mno-red-zone
include list.Makefile
FILES=$(LIBC_FILES)
OBJ=$(LIBC_OBJ)
SRCDEP=$(FILES)
INCLUDES=-I ../../include

##

.PHONY: all re clean libc.a

all: libc.a

%.o:%.c
	$(CC) $(FLAGS) -c $? -o $@ $(INCLUDES)

libc.a: $(OBJ)
	$(AR) rcs libc.a $(OBJ)

.depends:
	../../tools/mkdep.sh "$(CC) $(FLAGS)" $(SRCDEP) > .depends

include .depends

clean:
	@echo "Clean trash files"
	rm -rf $(NAME) $(OBJ) *.o *.a

re: clean $(NAME)
