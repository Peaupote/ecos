CC=x86_64-elf-gcc
FLAGS=-O2 -g -Wall -Wextra -std=gnu99 \
		-nostdlib -mcmodel=large -ffreestanding \
		-mno-red-zone
include list.Makefile
FILES=$(LIBC_FILES)
OBJ=$(LIBC_OBJ)
SRCDEP=$(FILES)
INCLUDES=-I ./include

##

.PHONY: all re clean

all: $(OBJ)

%.o:%.c
	$(CC) $(FLAGS) -c $? -o $@ $(INCLUDES)

.depends:
	../../tools/mkdep.sh "$(CC) $(FLAGS)" $(SRCDEP) > .depends

include .depends

clean:
	@echo "Clean trash files"
	rm -rf $(NAME) $(OBJ) *.o

re: clean $(NAME)
