ROOT=../../..
include ../vars.Makefile
FLAGS_K=$(FLAGS_T)

NAME=./test_ext2.out
IMG=../../run/home.img
INCLUDES=-I $(ROOT)/include

.PHONY: clean run ext2 img tutil

all:$(NAME)

ext2:
	$(MAKE) -C $(ROOT)/src/fs/ext2 ext2$(TSFX).a
	objcopy $(ROOT)/src/fs/ext2/ext2$(TSFX).a ./libext2.a

$(NAME): main.c ext2 ext2_wrap.o
	$(CC) $(FLAGS_T) -o $(NAME) main.c ext2_wrap.o $(INCLUDES) -L. -lext2

%.o:%.c
	$(CC) $(FLAGS_T) -o $@ -c $< $(INCLUDES)

img:
	$(MAKE) -C ../../run home.img

run: $(NAME) img
	 $(NAME) $(IMG)

clean:
	rm -rf *.o *.out *.s *.a $(NAME) $(OBJ)
