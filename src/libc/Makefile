NAME_LC?=libc
NAME_LK?=libk
CC?=x86_64-elf-gcc
AS?=x86_64-elf-as
AR?=x86_64-elf-ar
include ../../vars.Makefile

include list.Makefile
SRCDEP=$(LIBC_FILES) $(LIBK_FILES)
INCLUDES=-I ../../include

##

.PHONY: all re clean

all: libc.a libk.a

%.s:%.S
	$(CC) -E $? -o $@ $(INCLUDES)

sys.S:sys-as.awk ../../include/kernel/sys.h
	awk -f sys-as.awk ../../include/headers/sys.h > sys.S

%.$(NAME_LC).o:%.s
	$(AS) -c $? -o $@ $(INCLUDES)

%.$(NAME_LC).o:%.c
	$(CC) $(FLAGS_LC) -c $? -o $@ $(INCLUDES)

%.$(NAME_LK).o:%.s
	$(AS) -c $? -o $@ $(INCLUDES)

%.$(NAME_LK).o:%.c
	$(CC) $(FLAGS_LK) -c $? -o $@ $(INCLUDES)

$(NAME_LC).a: $(LIBC_OBJ)
	$(AR) rcs $@ $(LIBC_OBJ)

$(NAME_LK).a: $(LIBK_OBJ)
	$(AR) rcs $@ $(LIBK_OBJ)

.depends:
	../../tools/mkdep.sh "$(CC) $(FLAGS_L) $(INCLUDES)" $(SRCDEP) > .depends

include .depends

clean:
	@echo "Clean trash files"
	rm -rf $(NAME) $(OBJ) *.o **/*.o *.a *.s sys.S sys.h

re: clean $(NAME)
