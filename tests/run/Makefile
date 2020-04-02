ROOT=../..

.PHONY:clean sys home

home: sys
	mkdir -p home/test
	mkdir -p home/bin
	cp $(ROOT)/src/sys/init1.out home/bin/init1
	cp $(ROOT)/src/sys/sh.out    home/bin/sh
	cp $(ROOT)/src/sys/cat.out   home/bin/cat

home.img: home
	$(ROOT)/tools/build-ext2.sh home home.img

sys:
	$(MAKE) -C $(ROOT)/src/sys all

clean:
	rm -rf *.o *.img *.out *.s home
	$(MAKE) -C prg clean
