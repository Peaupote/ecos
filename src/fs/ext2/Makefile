CC?=gcc
AS?=as
AR?=ar
FLAGS?=-O2 -g -Wall -Wextra -std=gnu99 \
		-nostdlib -mcmodel=large -ffreestanding \
		-mno-red-zone
FILES=ext2.c inode.c dir.c
OBJ=$(FILES:%.c=%.o)
INCLUDES=-I ../../../include

##

.PHONY: all re clean

all: ext2.a

%.o:%.c
	$(CC) $(FLAGS) -c $? -o $@ $(INCLUDES)

ext2.a: $(OBJ)
	$(AR) rcs $@ $(OBJ)

.depends:
	../../../tools/mkdep.sh "$(CC) $(FLAGS)" $(SRCDEP) > .depends

include .depends

clean:
	@echo "Clean trash files"
	rm -rf $(NAME) $(OBJ) *.o **/*.o *.a *.s

re: clean $(NAME)
