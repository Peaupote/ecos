CC?=gcc
AS?=as
AR?=ar
LFLAGS?=-O2 -g -Wall -Wextra -std=gnu99 \
		-nostdlib -mcmodel=large -ffreestanding \
		-mno-red-zone
include list.Makefile
SRCDEP=$(LIBC_FILES) $(LIBK_FILES)
INCLUDES=-I ../../include

##

.PHONY: all re clean

all: libc.a libk.a

%.s:%.S
	$(CC) -E $? -o $@ $(INCLUDES)

sys.S:sys-as.awk ../../include/kernel/sys.h
	awk -f sys-as.awk < ../../include/headers/sys.h > sys.S

%.libc.o:%.s
	$(AS) -c $? -o $@ $(INCLUDES)

%.libc.o:%.c
	$(CC) $(LFLAGS) -D__is_libc -c $? -o $@ $(INCLUDES)

%.libk.o:%.s
	$(AS) -c $? -o $@ $(INCLUDES)

%.libk.o:%.c
	$(CC) $(LFLAGS) -D__is_libk -c $? -o $@ $(INCLUDES)

libc.a: $(LIBC_OBJ)
	$(AR) rcs $@ $(LIBC_OBJ)

libk.a: $(LIBK_OBJ)
	$(AR) rcs $@ $(LIBK_OBJ)

.depends:
	../../tools/mkdep.sh "$(CC) $(LFLAGS) $(INCLUDES)" $(SRCDEP) > .depends

include .depends

clean:
	@echo "Clean trash files"
	rm -rf $(NAME) $(OBJ) *.o **/*.o *.a *.s sys.S sys.h

re: clean $(NAME)
