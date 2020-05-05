ROOT=../..
STARTO="$(ROOT)/src/sys/start.o"

.PHONY: clean sys home $(STARTO)

unexport $(VARS_NAME)
include $(ROOT)/vars.Makefile
export CC?=x86_64-elf-gcc
export AS?=x86_64-elf-as

CFILES=test.c test2.c test3.c test_printf.c
OUT=$(CFILES:.c=.out) test_reg.out
SRCDEP=$(CFILES)
CPFILES=test.c demo.sh
INCLUDE_LIBC?=-I $(ROOT)/include/libc
INCLUDES=-I $(ROOT)/include $(INCLUDE_LIBC)

%.s:%.S
	$(CC) -E $< -o $@ $(INCLUDES)

%.out:%.s
	$(CC) $(FLAGS_UR) -o $@ $< $(INCLUDES) -lgcc

%.out:%.c libc $(STARTO)
	$(CC) $(FLAGS_UR) -o $@ $< $(STARTO) $(LINK_LC) $(INCLUDES) -lgcc

home.img: $(OUT)
	rm -rf home
	mkdir -p home
	mkdir -p home/test
	@$(foreach f,$(CPFILES),cp "$(f)" "home/test/$(f)" &&) echo $(CPFILES)
	@$(foreach o,$(OUT),cp "$(o)" "home/test/$(o:.out=)" &&) echo $(OUT)
	$(ROOT)/tools/build-ext2.sh home home.img

$(STARTO):
	$(MAKE) -C $(ROOT)/src/sys start.o

libc:
	$(MAKE) -C $(ROOT)/src/libc libc.sym

.depends:
	../../tools/mkdep.sh "$(CC) $(FLAGS_UR) $(INCLUDES)" \
		$(SRCDEP) > .depends

clean:
	rm -rf *.o *.img *.out *.s home
