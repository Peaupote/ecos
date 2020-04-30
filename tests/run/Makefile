ROOT=../..
STARTO="$(ROOT)/src/sys/start.o"

.PHONY: clean sys home $(STARTO)

unexport $(VARS_NAME)
include $(ROOT)/vars.Makefile

CFILES=test.c test2.c test3.c
OUT=$(CFILES:.c=.out)
SRCDEP=$(CFILES)
INCLUDE_LIBC?=-I $(ROOT)/include/libc
INCLUDES=-I $(ROOT)/include $(INCLUDE_LIBC)

%.out:%.c libc $(STARTO)
	$(CC) $(FLAGS_UR) -o $@ $< $(STARTO) \
		$(ROOT)/src/libc/libc.a $(INCLUDES) -lgcc

home.img: $(OUT)
	rm -rf home
	mkdir -p home
	mkdir -p home/test
	cp test.c home/test/test.c
	@$(foreach o,$(OUT),cp "$(o)" "home/test/$(o:.out=)" &&) echo $(OUT)
	$(ROOT)/tools/build-ext2.sh home home.img

$(STARTO):
	$(MAKE) -C $(ROOT)/src/sys start.o

libc:
	$(MAKE) -C $(ROOT)/src/libc libc.a

.depends:
	../../tools/mkdep.sh "$(CC) $(FLAGS_UR) $(INCLUDES)" \
		$(SRCDEP) > .depends

clean:
	rm -rf *.o *.img *.out *.s home
