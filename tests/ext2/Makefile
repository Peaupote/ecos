NAME=./test_ext2.out
CC=gcc
FLAGS=-O1 -Wall -Wextra -std=gnu99
INCLUDES=-I ../../include

.PHONY: run

all:$(NAME)

$(NAME): main.o
	$(CC) $(FLAGS) -o $(NAME) main.o $(INCLUDES)

%.o:%.c
	$(CC) $(FLAGS) -o $@ -c $< $(INCLUDES)

test.img:
	dd if=/dev/zero of=test.img bs=1M count=50
	mke2fs test.img

run: test.img
	$(NAME) test.img

clean:
	rm -f *.o *.img $(NAME)
