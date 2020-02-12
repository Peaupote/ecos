# Compilation parameters
NAME=kernel.o
CC=i686-elf-gcc
FLAGS=-O2 -g -Wall -Wextra -std=gnu99 -nostdlib -ffreestanding -mno-red-zone
FILES=proc.c file.c
OBJ=$(FILES:%.c=%.o)

##

.PHONY: all re clean

all: $(NAME)

%.o:%.c
	$(CC) $(FLAGS) -c $? -o $@

$(NAME): $(OBJ) main.c
	#$(CC) $(FLAGS) $(OBJ) main.c -o $(NAME)
	$(CC) -c fakekernel.c -o kernel.o $(FLAGS)

clean:
	@echo "Clean trash files"
	rm -rf $(NAME)
	rm -rf $(OBJ)

re: clean $(NAME)
