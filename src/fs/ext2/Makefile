export CC?=x86_64-elf-gcc
export AS?=x86_64-elf-as
export AR?=x86_64-elf-ar
ROOT=../../..
include $(ROOT)/vars.Makefile

FILES=ext2.c inode.c dir.c
SRCDEP=$(FILES)
OBJ=$(FILES:%.c=%$(TSFX).o)
INCLUDE_LIBC?=-I $(ROOT)/include/libc
INCLUDES=-I $(ROOT)/include $(INCLUDE_LIBC)
NAME=ext2$(TSFX).a

##

.PHONY: all re clean

all: $(NAME)

%$(TSFX).o:%.c
	$(CC) $(FLAGS_K) -c $< -o $@ $(INCLUDES)

$(NAME): $(OBJ)
	$(AR) rcs $@ $(OBJ)

$(TSFX).depends:
	$(ROOT)/tools/mkdep.sh "$(CC) $(FLAGS_K) $(INCLUDES)" $(SRCDEP) > .depends

include $(TSFX).depends

clean:
	@echo "Clean trash files"
	rm -rf $(NAME) $(OBJ) *.o **/*.o *.a *.s

re: clean all
