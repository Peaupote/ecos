export CC?=x86_64-elf-gcc
export AS?=x86_64-elf-as
export AR?=x86_64-elf-ar
ROOT=../../..
include $(ROOT)/vars.Makefile

FILES=ext2.c inode.c dir.c link.c
SRCDEP=$(FILES)
OBJ=$(FILES:%.c=%$(TSFX).o)
INCLUDE_LIBC?=-I $(ROOT)/include/libc
INCLUDES=-I $(ROOT)/include $(INCLUDE_LIBC)
NAME=ext2$(TSFX).a

##

.PHONY: all re clean clean-depends

all: $(NAME)

%$(TSFX).o:%.c
	$(CC) $(FLAGS_K) -c $< -o $@ $(INCLUDES)

$(NAME): $(OBJ)
	$(AR) rcs $@ $(OBJ)

$(TSFX).depends:
	$(ROOT)/tools/mkdep.sh "$(CC) $(FLAGS_K) $(INCLUDES)" $(SRCDEP) > .depends
clean-depends:
	rm -f *.depends .depends

include $(TSFX).depends

clean:
	rm -f $(NAME) $(OBJ) *.o **/*.o *.a *.s *.depends .depends

re: clean all
