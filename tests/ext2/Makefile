NAME=./test_ext2.out
export CC=gcc
export FLAGS=-O1 -Wall -Wextra -std=gnu99 -fno-builtin -D__is_test
INCLUDES=-I ../../include

.PHONY: run

.PHONY: clean run ext2 home

all:$(NAME)

ext2:
	$(MAKE) -C ../../src/fs/ext2 ext2.a

$(NAME): main.o $(OBJ) ext2
	$(CC) $(FLAGS) -o $(NAME) ../../src/fs/ext2/ext2.a main.o $(INCLUDES)

%.o:%.c
	$(CC) $(FLAGS) -o $@ -c $< $(INCLUDES)

home: bio.txt
	mkdir -p home/mpouzet
	cp bio.txt home/mpouzet/bio.txt
	x86_64-elf-gcc -E hello-world.S -o home/mpouzet/hello-world.s $(INCLUDES)
	x86_64-elf-as home/mpouzet/hello-world.s -o home/mpouzet/hello-world.out
	echo "Hello world !" > home/mpouzet/hello-world

home.img: home
	./build-home.sh

run: $(NAME) home.img
	# $(NAME) home.img

clean:
	rm -rf *.o *.img *.out *.s home $(NAME) $(OBJ)
