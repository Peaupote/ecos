NAME=./test_ext2.out
export CC=gcc
export FLAGS=-O2 -Wall -Wextra -std=gnu99 -fno-builtin -D__is_test
INCLUDES=-I ../../include

.PHONY: run

.PHONY: clean run ext2 home aux_prg

all:$(NAME)

ext2:
	$(MAKE) -C ../../src/fs/ext2 ext2.a

$(NAME): main.c ext2
	$(CC) $(FLAGS) -o $(NAME) ../../src/fs/ext2/*.o main.c $(INCLUDES)

#%.o:%.c
#	$(CC) $(FLAGS) -o $@ -c $< $(INCLUDES)

home: aux_prg
	mkdir -p home/test
	cp ../../src/kernel/aux_prg/init1.out home/test/init1.out

home.img: home
	./build-home.sh

aux_prg:
	$(MAKE) -C ../../src/kernel/aux_prg all

run: $(NAME) home.img
	 $(NAME) home.img

clean:
	rm -rf *.o *.img *.out *.s home $(NAME) $(OBJ)
