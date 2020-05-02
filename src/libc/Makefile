CC?=x86_64-elf-gcc
AS?=x86_64-elf-as
AR?=x86_64-elf-ar
OBJCP?=x86_64-elf-objcopy
ROOT=../..
include $(ROOT)/vars.Makefile

include list.Makefile
INCLUDES=-I $(ROOT)/include

##

.PHONY: all re clean clean-depends elf_info

all: libc.out libc.sym libk.a

%.o:%.S

%.s:%.S
	$(CC) -E $< -o $@ $(INCLUDES)

# Libc

%.$(NAME_LC).o:%.s
	$(AS) -c $< -o $@ $(INCLUDES)

%.$(NAME_LC).o:%.c
	$(CC) $(FLAGS_LC) -c $< -o $@ $(INCLUDES)

$(NAME_LC).a: $(LIBC_OBJ)
	$(AR) rcs $@ $(LIBC_OBJ)

$(NAME_LC).bin: $(NAME_LC).a linker.ld
	$(CC) $(FLAGS_LC) -T linker.ld -o $@ $(LIBC_OBJ)

$(NAME_LC).out: $(NAME_LC).bin elf_info
	$(ROOT)/tools/elf_info.out c $(NAME_LC).bin $@

$(NAME_LC).sym: $(NAME_LC).bin
	$(OBJCP) --extract-symbol $(NAME_LC).bin $@

$(NAME_LC).depends:
	../../tools/mkdep.sh "$(CC) $(FLAGS_L) $(INCLUDES)" $(LIBC_DEP)\
		> $(NAME_LC).depends

include $(NAME_LC).depends

# Libk

%.$(NAME_LK).o:%.s
	$(AS) -c $< -o $@ $(INCLUDES)

%.$(NAME_LK).o:%.c
	$(CC) $(FLAGS_LK) -c $< -o $@ $(INCLUDES)

$(NAME_LK).a: $(LIBK_OBJ)
	$(AR) rcs $@ $(LIBK_OBJ)

$(NAME_LK).depends:
	../../tools/mkdep.sh "$(CC) $(FLAGS_L) $(INCLUDES)" $(LIBK_DEP)\
		> $(NAME_LK).depends

include $(NAME_LK).depends

##

elf_info:
	$(MAKE) -C $(ROOT)/tools elf_info.out

clean-depends:
	rm -f *.depends .depends

clean:
	rm -f $(NAME) $(OBJ) *.out *.bin *.o **/*.o *.a *.s *.sym \
		sys.h *.depends .depends

re: clean $(NAME)
