NAME=libc.bin
CC=x86_64-elf-gcc
FLAGS=-O2 -g -Wall -Wextra -std=gnu99 \
		-nostdlib -ffreestanding \
		-mno-red-zone
FILES=string/memcmp.c \
	  string/memcpy.c \
	  string/memset.c \
	  string/memmove.c \
	  string/strcpy.c \
	  string/strlen.c \
	  stdio/printf.c \
	  stdio/putchar.c \
	  stdio/puts.c
OBJ=$(FILES:%.c=%.o)
INCLUDES=-I ./include

##

.PHONY: all re clean

all: $(NAME)

%.o:%.c
	$(CC) $(FLAGS) -c $? -o $@ $(INCLUDES)

$(NAME): $(OBJ)

clean:
	@echo "Clean trash files"
	rm -rf $(NAME) $(OBJ) *.o

re: clean $(NAME)
