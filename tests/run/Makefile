ROOT=../..

.PHONY:clean sys home

include $(ROOT)/vars.Makefile
unexport $(VARS_NAME)

home: sys
	rm   -rf home
	mkdir -p home/test
	mkdir -p home/bin
	cp $(ROOT)/src/sys/init1.out home/bin/init1
	cp $(ROOT)/src/sys/sh.out    home/bin/sh
	cp $(ROOT)/src/sys/cat.out   home/bin/cat
	cp $(ROOT)/src/sys/echo.out  home/bin/echo
	cp $(ROOT)/src/sys/stat.out  home/bin/stat
	cp $(ROOT)/src/sys/ls.out    home/bin/ls
	cp $(ROOT)/src/sys/ps.out    home/bin/ps
	cp $(ROOT)/src/sys/wc.out    home/bin/wc

	cp $(ROOT)/src/sys/test.c    home/test/test.c
	cp $(ROOT)/src/sys/test.out  home/test/test

home.img: home
	$(ROOT)/tools/build-ext2.sh home home.img

sys:
	$(MAKE) -C $(ROOT)/src/sys all

clean:
	rm -rf *.o *.img *.out *.s home
	$(MAKE) -C prg clean
