export CC?=x86_64-elf-gcc
export AS?=x86_64-elf-as
ROOT=../..
include $(ROOT)/vars.Makefile

include list.Makefile
INCLUDE_LIBC?=-I ../../include/libc
INCLUDES=-I ../../include $(INCLUDE_LIBC)

.PHONY: all clean libc

%.o:%.S

%.s:%.S
	$(CC) -E $< -o $@ $(INCLUDES)

%.o:%.s
	$(AS) $< -o $@ $(INCLUDES)

%.out:%.c libc start.o 
	$(CC) $(FLAGS_UR) -o $@ $< start.o $(LINK_LC) $(INCLUDES) -lgcc

sh.out:sh/sh.h $(CFILES_SH) libc start.o
	$(CC) $(FLAGS_UR) -o $@ $(CFILES_SH) $< start.o \
		$(LINK_LC) $(INCLUDES) -lgcc

root.img:$(OUT) $(SHFILES)
	@rm   -rf root
	@mkdir -p root
	@mkdir -p root/home
	@mkdir -p root/bin
	@mkdir -p root/proc
	@$(foreach o,$(OUT),cp "$(o)" "root/bin/$(o:.out=)" &&) echo $(OUT)
	@$(foreach s,$(SHFILES),cp "$(s)" "root/bin/$(s:.sh=)" &&) echo $(SHFILES)
	$(ROOT)/tools/build-ext2.sh root root.img

all:root.img

libc:
	$(MAKE) -C ../libc libc.sym

.depends:
	../../tools/mkdep.sh "$(CC) $(FLAGS_UR) $(INCLUDES)" \
		$(SRCDEP) > .depends

include .depends

clean:
	rm -f *.out *.s *.o .depends *.img
	rm -rf ./root
