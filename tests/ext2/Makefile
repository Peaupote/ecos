NAME=./test_ext2.out
CC=gcc
FLAGS=-O1 -Wall -Wextra -std=gnu99 -fno-builtin
FILES=$(addprefix ../../src/fs/ext2/, ext2.c inode.c dir.c) \
	  ../../src/util/string.o
OBJ=$(FILES:.c=.o)
INCLUDES=-I ../../include

.PHONY: run

all:$(NAME)

$(NAME): main.o $(OBJ)
	$(CC) $(FLAGS) -o $(NAME) $(OBJ) main.o $(INCLUDES)

%.o:%.c
	$(CC) $(FLAGS) -o $@ -c $< $(INCLUDES)

test.img:
	dd if=/dev/zero of=test.img bs=1M count=50
	mke2fs test.img

run: $(NAME) test.img
	$(NAME) test.img

clean:
	rm -f *.o *.img $(NAME) $(OBJ)
