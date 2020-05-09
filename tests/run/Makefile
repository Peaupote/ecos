ROOT=../..
STARTO="$(ROOT)/src/sys/start.o"

.PHONY: clean sys home $(STARTO)

unexport $(VARS_NAME)
include $(ROOT)/vars.Makefile
export CC?=x86_64-elf-gcc
export AS?=x86_64-elf-as

include list.Makefile
INCLUDE_LIBC?=-I $(ROOT)/include/libc
INCLUDES=-I $(ROOT)/include $(INCLUDE_LIBC)

%.s:%.S
	$(CC) -E $< -o $@ $(INCLUDES)

%.out:%.s
	$(CC) $(FLAGS_UR) -o $@ $< $(INCLUDES) -lgcc

%.out:%.c libc $(STARTO)
	$(CC) $(FLAGS_UR) -o $@ $< $(STARTO) $(LINK_LC) $(INCLUDES) -lgcc

%.o:%.c
	$(CC) $(FLAGS_UR) -o $@ -c $< $(INCLUDES)

demo/mat.out:$(OFILES_MAT) libc $(STARTO)
	$(CC) $(FLAGS_UR) -o $@ $(OFILES_MAT) $(STARTO) \
		$(LINK_LC) $(INCLUDES) -lgcc


home.img: $(OUT) $(CPFILES)
	rm -rf home
	mkdir -p home
	mkdir -p home/test
	mkdir -p home/test/demo
	@$(foreach f,$(CPFILES),cp "$(f)" "home/test/$(f)" &&) echo $(CPFILES)
	@$(foreach d,$(CPDIRS),cp -r "$(d)" "home/test/$(d)" &&) echo $(CPDIRS)
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
	rm -rf *.o *.img *.out *.s home $(CLEANF)
